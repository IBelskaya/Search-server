#pragma once

#include "document.h"
#include <string>
#include <vector>
#include <string_view>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double precision = 1e-6;
std::string ReadLine(); 
int ReadLineWithNumber();   
void PrintDocument(const Document& document);
void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status);