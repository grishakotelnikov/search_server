#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(std::string_view(stop_words_text))  // Invoke delegating constructor
// from string container
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    std::unordered_set<std::string> words_values;
    for (const std::string_view word : words) {
        auto it = words_values.emplace(std::string(word));
        std::string_view word_view = *it.first;
        word_to_document_freqs_[word_view][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word_view] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, std::move(words_values) });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}
int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, true);
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { std::vector<std::string_view>(), documents_.at(document_id).status };
        }
    }
    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    for_each(query.plus_words.begin(), query.plus_words.end(),
             [this, document_id, &matched_words](auto& word) {
                 if (document_to_word_freqs_.at(document_id).count(word) != 0) {
                     matched_words.push_back(word);
                 }
             });
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { std::vector<std::string_view>(), documents_.at(document_id).status };
        }
    }
    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());

    for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
             [this, document_id, &matched_words](auto& word) {
                 if (document_to_word_freqs_.at(document_id).count(word) != 0) {
                     matched_words.push_back(word);
                 }
             });
    const auto word_sv = [this, document_id](std::string_view word){
        auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };
    auto word_end = copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), word_sv);
    sort(std::execution::par, matched_words.begin(), word_end);
    matched_words.erase(unique(std::execution::par, matched_words.begin(), word_end), matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

const std::map<std::string_view, double, std::less<>>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double, std::less<>> empty_result;
    if (document_to_word_freqs_.count(document_id) == 0) {
        return empty_result;
    }
    return document_to_word_freqs_.at(document_id);
}
const std::set<int>::const_iterator  SearchServer::begin() const
{
    return document_ids_.begin();
}

const std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}


bool SearchServer::IsStopWord(const std::string_view word) const  {
    return stop_words_.count(word) > 0;
}
bool SearchServer::IsValidWord(const std::string_view word) {

    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}
std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text.remove_prefix(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word is invalid");
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool sort ) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (sort) {
        std::sort(std::execution::par, result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(unique(std::execution::par, result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());
       std::sort(std::execution::par, result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(unique(std::execution::par, result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

