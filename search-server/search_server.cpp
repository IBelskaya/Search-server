#include "search_server.h"
#include <numeric> 
#include <execution>

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  
{
}
SearchServer::SearchServer(std::string_view stop_words_text)
       : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,const std::vector<int>& ratings)
{
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw std::invalid_argument("Invalid document_id"s);
        }
        const auto [it, inserted] = documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, std::string(document) });
    	const auto words = SplitIntoWordsNoStop(it->second.str);
           // const auto words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (std::string_view word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
            document_to_word_freqs_[document_id][word] += inv_word_count;
        }
       // documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_ids_.insert(document_id);
 }

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static  std::map<std::string_view, double> emply_map;
    if (documents_.count(document_id) == 0) {
        return emply_map;
    }
    return  document_to_word_freqs_.at(document_id);
}     
 void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id); 
}
void SearchServer:: RemoveDocument(const std::execution::sequenced_policy&, int document_id){
    if (documents_.count(document_id)!=0){
      
        for (auto& word : document_to_word_freqs_.at(document_id)) {
  word_to_document_freqs_[word.first].erase(document_id);
             }
        document_ids_.erase(document_id);
        documents_.erase(document_id); 
        document_to_word_freqs_ .erase(document_id);    
    }   
}
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) { 
    if (documents_.count(document_id)) {
        std::map<std::string_view, double>* word_freqs = &document_to_word_freqs_[document_id];
        std::vector<std::string_view> words(word_freqs->size());         transform(
           std::execution::par,
           word_freqs->begin(), word_freqs->end(),
           words.begin(),
           [](const auto& item) {return std::move(item.first); }
        ); 
        for_each(
            std::execution::par,
            words.begin(), words.end(),
            [this, document_id](auto& word) {
            word_to_document_freqs_[word].erase(document_id);
        });
        document_to_word_freqs_.erase(document_id);
        document_ids_.erase(document_id);
        documents_.erase(document_id);
    }
}
  
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::set<int>::iterator SearchServer::begin( ){
    return document_ids_.begin();
}
    
const std::set<int>::iterator  SearchServer::end( )  {
    return  document_ids_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {
    if ((document_id < 0) || (documents_.count(document_id)==0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    if (!IsValidWord(raw_query)) {
        throw std::invalid_argument("Some of query words are invalid"s);
    }
    const auto query = ParseQuery(raw_query);
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { {}, documents_.at(document_id).status };
        }
    }
    std::vector<std::string_view> matched_words;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return {matched_words, documents_.at(document_id).status};
}
  
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, true);
    const auto status = documents_.at(document_id).status;
    const auto word_checker   =
        [this, document_id](const std::string_view word) {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), word_checker )) {
        return { {}, status };
    }
 
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto words_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(), word_checker
    );
    std::sort(matched_words.begin(), words_end);
    words_end = std::unique(matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());
 
    return { matched_words, status };
}

bool SearchServer::IsStopWord(std::string_view word) const {
        return stop_words_.count(std::string(word)) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
        std::vector<std::string_view> words;
        for (auto word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()){
            return 0;
        }
        int rating_sum = std::accumulate(ratings.begin(), ratings.end(),0); 
        return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    using namespace std::literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool no_sort) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!no_sort) {
        for (auto* words : { &result.plus_words, &result.minus_words }) {
            std::sort(words->begin(), words->end());
            words->erase(unique(words->begin(), words->end()), words->end());
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
