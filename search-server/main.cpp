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
    ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "There are NO documents yet");
    server.AddDocument(3, "I love my pretty cat", DocumentStatus::ACTUAL, {1,2,3});
    ASSERT_EQUAL(server.GetDocumentCount(), 1);
    server.AddDocument(4, "buz buster kop", DocumentStatus::ACTUAL, {3,4,5});
    ASSERT_EQUAL(server.GetDocumentCount(), 2);
    server.AddDocument(5, "bu buster kop", DocumentStatus::ACTUAL, {5,3,1});
    ASSERT_EQUAL(server.GetDocumentCount(), 3);
}


void TestMatchDoc()
{
    SearchServer server;
    server.AddDocument(1, "bill kill ball", DocumentStatus::ACTUAL, {1,2});
    server.FindTopDocuments("ball");
    const auto ad = server.MatchDocument("ball", 1);

    const tuple<vector<string>,DocumentStatus> af({"ball"}, DocumentStatus::ACTUAL);
    ASSERT(af == ad);
}

void TestDeleteDocumentsWithMinusWords() {
    {
        SearchServer server;
        server.AddDocument(1, "I love my pretty cat"s, DocumentStatus::ACTUAL, {1, 2});
        const auto found_docs = server.FindTopDocuments("cat -dog"s);
        ASSERT_EQUAL(found_docs.size(), 1);
    }


    {
        SearchServer server;
        server.AddDocument(2, "I love my pretty cat"s, DocumentStatus::ACTUAL, {3, 4, 5});
        const auto found_docs = server.FindTopDocuments("pretty -cat");
        ASSERT(found_docs.empty());
    }
}
void TestGetRightStatus()
{
    {
        SearchServer server;
        server.AddDocument(1, "I love my darling cat", DocumentStatus::BANNED, {1, 2});
        server.AddDocument(2, "I love my dog", DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(3, "I love my bird", DocumentStatus::ACTUAL, {1, 3});
        const auto found_docs = server.FindTopDocuments("love", DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), 1);
    }

    {
        SearchServer server;
        server.AddDocument(1, "I love my darling cat", DocumentStatus::BANNED, {1, 2});
        server.AddDocument(2, "I love my dog", DocumentStatus::ACTUAL, {1, 2, 3});
        server.AddDocument(3, "I love my bird", DocumentStatus::ACTUAL, {1, 3});
        const auto found_docs = server.FindTopDocuments("I love");
        ASSERT_EQUAL(found_docs.size(), 2);
    }


}
void TestRatingCalculation() {
    {
        SearchServer server;
        server.AddDocument(1, "I love my darling", DocumentStatus::ACTUAL, {1, 2});
        const auto found_docs = server.FindTopDocuments("love");
        auto id_1 = found_docs[0];
        ASSERT_EQUAL(id_1.rating, 1);
    }

    {
        SearchServer server;
        server.AddDocument(2, "I love my dog", DocumentStatus::ACTUAL, {1, 2, 6});
        const auto found_docs = server.FindTopDocuments("love");
        auto id_1 = found_docs[0];
        ASSERT_EQUAL(id_1.rating, 3);
    }

    {
        SearchServer server;
        server.AddDocument(3, "I love my bird", DocumentStatus::ACTUAL, {1, 3});
        const auto found_docs = server.FindTopDocuments("love");
        auto id_1 = found_docs[0];
        ASSERT_EQUAL(id_1.rating, 2);
    }

}


void TestSortRelevance() {
    SearchServer server;
    server.AddDocument(2, "I love my dog", DocumentStatus::ACTUAL, {1, 2, 6});
    server.AddDocument(3, "I love my cat", DocumentStatus::ACTUAL, {1, 6});

    const auto found_docs = server.FindTopDocuments("love my dog");
    auto id_1 = found_docs[0];
    auto id_2 = found_docs[1];

    ASSERT(id_1.relevance > id_2.relevance);

}




void TestSearchServer() {

    TestExcludeStopWordsFromAddedDocumentContent();
    TestDocumentCount();
    TestDeleteDocumentsWithMinusWords();
    TestMatchDoc();
    TestGetRightStatus();
    TestRatingCalculation();
    TestSortRelevance();

}


