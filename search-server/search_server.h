#pragma once
#include "read_input_functions.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include "document.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <tuple>  
#include <set> 
#include <map>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <execution>
#include <string_view>
 
using namespace std::string_literals;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }
   
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);
       
    void AddDocument(int document_id, const std::string& document, DocumentStatus status,const std::vector<int>& ratings);
     
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const; 
      
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id); 
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
                                      DocumentPredicate document_predicate) const{
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const  ExecutionPolicy& policy, std::string_view raw_query,  DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);
 
        auto matched_documents = FindAllDocuments(policy, query, document_predicate);
 
        sort(policy, matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
            return lhs.relevance > rhs.relevance
            || (std::abs(lhs.relevance - rhs.relevance) < precision  && lhs.rating > rhs.rating);
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const  ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const{
        return FindTopDocuments(policy, raw_query, 
               [status](int document_id, DocumentStatus document_status, int rating) {
               return document_status == status;});     
    }
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const  ExecutionPolicy& policy, std::string_view raw_query) const{
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);    
    }
    
    int GetDocumentCount() const;
    const std::set<int>::iterator  begin( );
    const std::set<int>::iterator  end( );
       
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const; 
    
    private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string str;
    };
   
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>>  document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    
    bool IsStopWord(std::string_view word) const;
    static bool IsValidWord(std::string_view word);
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
    
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(std::string_view text) const;
    
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
   
    
    Query ParseQuery(std::string_view text, bool no_sort = false) const;
      
    double ComputeWordInverseDocumentFreq(std::string_view word) const;
   
 
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy policy, const Query& query, 
                                          DocumentPredicate document_predicate) const {
        ConcurrentMap<int, double> document_to_relevance(7);

        const auto plus_word_checker =
            [this, &document_predicate, &document_to_relevance](std::string_view word) {
            if (word_to_document_freqs_.count(word) == 0) {
                return;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += static_cast<double>(term_freq * inverse_document_freq);
                }
            }
        };
        std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), plus_word_checker);

        const auto minus_word_checker =
            [this, &document_predicate, &document_to_relevance](std::string_view word) {
            if (word_to_document_freqs_.count(word) == 0) {
                return;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.Erase(document_id);
            }
        };
        std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), minus_word_checker);

        std::map<int, double> result = document_to_relevance.BuildOrdinaryMap();
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : result) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }  
};
