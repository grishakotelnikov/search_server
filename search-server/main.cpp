#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;




void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

void TestDocumentCount()
{
    SearchServer server;
    server.AddDocument(3,"I love my pretty cat",DocumentStatus::ACTUAL,{1,2,3});
    server.AddDocument(4,"buz buster kop",DocumentStatus::ACTUAL,{3,4,5});
    server.AddDocument(5,"bu buster kop",DocumentStatus::ACTUAL,{5,3,1});
    int current_document_count =(server.GetDocumentCount()==3);
    ASSERT_HINT(current_document_count,"Wrong server document count");

}


void TestMatchDoc()
{
    SearchServer server;
    server.AddDocument(1,"bill kill ball",DocumentStatus::ACTUAL,{1,2});
    server.FindTopDocuments("ball");
    const auto ad=server.MatchDocument("ball",1);

    const tuple<vector<string>,DocumentStatus> af({"ball"},DocumentStatus::ACTUAL);
    ASSERT(af==ad);

}

void TestMinusSlov()
{
    SearchServer server;
    server.AddDocument(1, "I love my pretty cat", DocumentStatus::ACTUAL, {1, 2});
    server.FindTopDocuments("big pretty dog -cat");
    const auto ad = server.MatchDocument("big pretty dog -cat", 1);

    const tuple<vector<string>, DocumentStatus> af;
    ASSERT(af == ad);
}
void TestStatus()
{
    SearchServer server;
    server.AddDocument(1, "bill kill ball", DocumentStatus::BANNED, {1, 2});
    ASSERT(server.FindTopDocuments("ball").empty());

}

void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestDocumentCount();
    TestMinusSlov();
    TestMatchDoc();
    TestStatus();
}



