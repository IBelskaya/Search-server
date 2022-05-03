#include "process_queries.h"
#include <vector>
#include <list>
using namespace std;
 
vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries)
{   vector<vector<Document>> result(queries.size());
    transform(execution::par,queries.begin(), queries.end(), result.begin(),
        [&search_server](const string& query) { return search_server.FindTopDocuments(query); });
    return result;
}
vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    vector<Document> result;
    for ( const auto& documents : ProcessQueries(search_server, queries)  ){
          result.insert(result.end(),documents.begin(), documents.end());
    }
      return result;
}
