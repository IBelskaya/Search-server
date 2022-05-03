#pragma once
#include "document.h"
#include "search_server.h"
#include <algorithm>
#include <execution>
#include <list>
using namespace std;
 
vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries);
vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries);
