#include "remove_duplicates.h"
#include <iostream>
#include <utility>

void RemoveDuplicates(SearchServer& search_server){
    std::vector <int> duplicdoc_id;
    std::set <std::vector<std::string>> doc;
    for(const int id:search_server){
        std::map <std::string, double> document=search_server.GetWordFrequencies(id);
        std::vector<std::string> d;
        transform (document.begin(), document.end(), inserter(d, d.begin()), [](const std::pair <std::string, double>& key_value) { return key_value.first; }); 
        if (doc.count(d)==1) {
            duplicdoc_id.push_back(id);
        } else {doc.emplace(d);}
    } 
    for(int document_id : duplicdoc_id){
        search_server.RemoveDocument(document_id); 
        std::cout<<"Found duplicate document id "<<document_id<<std::endl;
    }
}     
