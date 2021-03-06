#include "read_input_functions.h"
#include <iostream>
using namespace std::string_literals;
std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}
 
int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << std::endl;
}
 
void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    std::cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const std::string_view& word : words) {
        std::cout << ' ' << std::string(word);
    }
    std::cout << "}"s << std::endl;
}