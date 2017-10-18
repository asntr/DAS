#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <algorithm>
#include <unordered_map>

class Automaton {
public:
    struct TrieNode {
        std::unordered_map<int, std::shared_ptr<TrieNode>> next;
        std::unordered_map<int ,std::shared_ptr<TrieNode>> transition;
        std::shared_ptr<TrieNode> suffix_link;
        std::shared_ptr<TrieNode> parent;
        int pc;
        std::vector<int> pattern_ids;

        TrieNode(std::shared_ptr<TrieNode> _parent = nullptr, int _pc = -1) :parent(_parent), suffix_link(nullptr), pc(_pc)
        {}
    };

    std::shared_ptr<TrieNode> init(const std::vector<std::string>& patterns) {
        root_ = std::make_shared<TrieNode>();
        add_strings(patterns);
        add_suffix_links();
        return root_;
    }

    std::shared_ptr<TrieNode> go(std::shared_ptr<TrieNode> state, int c) {
        if (state->transition.find(c) == state->transition.end()) {
            if (state->next.find(c) != state->next.end()) state->transition[c] = state->next[c];
            else {
                if (state == root_) state->transition[c] = state;
                else state->transition[c] = go(state->suffix_link, c);
            }
        }
        return state->transition[c];
    }

    void destroy() {
        root_.reset();
    }
private:

    void add_string(const std::string& pattern, int id) {
        std::shared_ptr<TrieNode> cur = root_;
        for (int i = 0; i < pattern.size(); ++i) {
            char c = pattern[i];
            if (cur->next.find(c) == cur->next.end()) cur->next[c] = std::make_shared<TrieNode>(cur, c);
            cur = cur->next[c];
        }
        cur->pattern_ids.push_back(id);
    }

    void add_strings(const std::vector<std::string>& patterns) {
        for (int i = 0; i < patterns.size(); ++i) {
            add_string(patterns[i], i);
        }
    }

    void add_suffix_link(std::shared_ptr<TrieNode> node) {
        if (node == root_) {
            node->suffix_link = node;
            return;
        }

        auto u = node->parent;
        char c = node->pc;
        do {
            u = u->suffix_link;
        } while (u != root_ && u->next.find(c) == u->next.end());
        if (u->next.find(c) != u->next.end() && u->next[c] != node) node->suffix_link = u->next[c];
        else node->suffix_link = u;
    }

    void add_suffix_links() {
        std::queue<std::shared_ptr<TrieNode>> queue;
        queue.push(root_);
        while (!queue.empty()) {
            auto node = queue.front();
            for (auto child : node->next) {
                queue.push(child.second);
            }
            add_suffix_link(node);
            queue.pop();
        }
    }

    std::shared_ptr<TrieNode> root_;
};

class PatternsPreprocessor {
public:
    void preprocess(const std::vector<std::vector<std::string>>& patterns, int number_of_rows) {
        std::unordered_map<std::string, int> tagger;
        tagger.reserve(number_of_rows);
        unique_rows.reserve(number_of_rows);
        pattern_patterns_.resize(patterns.size());
        int rows_in_pattern = patterns[0].size();
        for (int i = 0; i < patterns.size(); ++i) {
            pattern_patterns_[i].resize(rows_in_pattern);
            for (int j = 0; j < patterns[i].size(); ++j) {
                if (tagger.find(patterns[i][j]) == tagger.end()) {
                    int index = tagger.size();
                    tagger[patterns[i][j]] = index;
                    unique_rows.push_back(patterns[i][j]);
                }
                pattern_patterns_[i][j] = (tagger[patterns[i][j]] + 1);
            }
        }
    }

    std::vector<std::string> get_unique_rows() {
        return unique_rows;
    }

    std::vector<std::string> get_pattern_patterns() {
        return pattern_patterns_;
    }

    void clear() {
        unique_rows.clear();
        pattern_patterns_.clear();
    }
private:
    std::vector<std::string> unique_rows;
    std::vector<std::string> pattern_patterns_;
};

class PatternMatcher {
public:
    PatternMatcher(const std::vector<std::string>& rows, std::vector<std::string>* text, int n, int m, int t, int k, int l) :
        n_(n), m_(m), t_(t), k_(k), l_(l) {
        text_ = text;
        initial_state_ = rows_automaton_.init(rows);
    }

    std::vector<int> get_count(const std::vector<std::string>& pattern_patterns) {
        fill_tags();
        Automaton a;
        auto start = a.init(pattern_patterns);
        std::vector<int> counter(t_, 0);
        for (int j = 0; j < m_; ++j) {
            auto cur_state = start;
            for (int i = 0; i < n_; ++i) {
                int c = (*text_)[i][j];
                cur_state = a.go(cur_state, c);
                if (!cur_state->pattern_ids.empty()) {
                    for (int id : cur_state->pattern_ids) {
                        ++counter[id];
                    }
                }
            }
        }
        return counter;
    }
private:
    void fill_tags() {
        for (int i = 0; i < n_; ++i) {
            auto cur_state = initial_state_;
            for (int j = 0; j < m_; ++j) {
                cur_state = rows_automaton_.go(cur_state, (*text_)[i][j]);
                if (!cur_state->pattern_ids.empty()) {
                    (*text_)[i][j] = cur_state->pattern_ids[0] + 1;
                } else {
                    (*text_)[i][j] = 0;
                }
            }
        }
        initial_state_.reset();
        rows_automaton_.destroy();
    }
    int n_, m_, t_, k_, l_;
    Automaton rows_automaton_;
    std::vector<std::string>* text_;
    std::shared_ptr<Automaton::TrieNode> initial_state_;
};

int main() {
    int n, m, t, k, l;
    std::vector<std::string> text;
    std::vector<std::vector<std::string>> patterns;
    std::cin >> n >> m;
    text.resize(n);
    for (int i = 0; i < n; ++i) {
        text[i].resize(m);
        for (int j = 0; j < m; ++j) {
            std::cin >> text[i][j];
        }
    }
    std::cin >> t >> k >> l;
    patterns.resize(t);
    for (int i = 0; i < t; ++i) {
        patterns[i].resize(k);
        for (int j = 0; j < k; ++j) {
            patterns[i][j].resize(l);
            for (int p = 0; p < l; ++p) {
                std::cin >> patterns[i][j][p];
            }
        }
    }
    PatternsPreprocessor pp;
    pp.preprocess(patterns, k);
    auto pattern_patterns = pp.get_pattern_patterns();
    auto unique_rows = pp.get_unique_rows();
    pp.clear();
    PatternMatcher pm(unique_rows, &text, n, m, t, k, l);
    auto res = pm.get_count(pattern_patterns);
    for (int i : res) {
        std::cout << i << " ";
    }
    return 0;
}