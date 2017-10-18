#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <queue>

struct TrieNode {
    std::vector<std::shared_ptr<TrieNode>> next;
    std::vector<std::shared_ptr<TrieNode>> transition;
    std::shared_ptr<TrieNode> suffix_link;
    std::shared_ptr<TrieNode> parent;
    char pc;
    bool terminal;
    size_t pattern_id;
    bool checked;
    std::vector<size_t> patterns_ids;

    TrieNode(std::shared_ptr<TrieNode> _parent = nullptr, char _pc = -1): next(26, nullptr), transition(26, nullptr), terminal(false), parent(_parent), suffix_link(nullptr), pc(_pc), checked(false)
    {}
};

void add_strings(std::shared_ptr<TrieNode> root, const std::vector<std::string>& patterns);
void add_suffix_links(std::shared_ptr<TrieNode> root);

std::shared_ptr<TrieNode> create_trie(const std::vector<std::string> patterns) {
    auto root = std::make_shared<TrieNode>();
    add_strings(root, patterns);
    add_suffix_links(root);
    return root;
}

void add_string(std::shared_ptr<TrieNode> root, const std::string& pattern, int id) {
    std::shared_ptr<TrieNode> cur = root;
    for (int i = 0; i < pattern.size(); ++i) {
        char c = pattern[i] - 'a';
        if (cur->next[c] == nullptr) cur->next[c] = std::make_shared<TrieNode>(cur, c);
        cur = cur->next[c];
    }
    cur->terminal = true;
    cur->pattern_id = id;
}

void add_strings(std::shared_ptr<TrieNode> root, const std::vector<std::string>& patterns) {
    for (int i = 0; i < patterns.size(); ++i) {
        add_string(root, patterns[i], i);
    }
}

void add_suffix_link(std::shared_ptr<TrieNode> node) {
    if (node->parent == nullptr) {
        node->suffix_link = node;
        return;
    }

    auto u = node->parent;
    char c = node->pc;
    do {
        u = u->suffix_link;
    } while (u->parent != nullptr && u->next[c] == nullptr);
    if (u->next[c] != nullptr && u->next[c] != node) node->suffix_link = u->next[c];
    else node->suffix_link = u;
}

void add_suffix_links(std::shared_ptr<TrieNode> root) {
    std::queue<std::shared_ptr<TrieNode>> queue;
    queue.push(root);
    while (!queue.empty()) {
        auto node = queue.front();
        for (auto child : node->next) {
            if (child != nullptr) queue.push(child);
        }
        add_suffix_link(node);
        queue.pop();
    }
}

std::shared_ptr<TrieNode> get_transition(std::shared_ptr<TrieNode> node, char c) {
    if (node->transition[c] == nullptr) {
        if (node->next[c] != nullptr) node->transition[c] = node->next[c];
        else {
            if (node->parent == nullptr) node->transition[c] = node;
            else node->transition[c] = get_transition(node->suffix_link, c);
        }
    }
    return node->transition[c];
}

void check(std::shared_ptr<TrieNode> node, std::vector<bool>& flagman) {
    if (node->checked) {
        for (auto id : node->patterns_ids) {
            flagman[id] = true;
        }
    } else {
        auto cur_node = node;
        while (cur_node->parent != nullptr) {
            if (cur_node->terminal) {
                flagman[cur_node->pattern_id] = true;
                node->patterns_ids.push_back(cur_node->pattern_id);
            }
            cur_node = cur_node->suffix_link;
        }
        node->checked = true;
    }
}

void process_word(const std::string& s, std::vector<bool>& flagman, std::shared_ptr<TrieNode> root) {
    auto cur_node = root;
    for (int i = 0; i < s.size(); ++i) {
        cur_node = get_transition(cur_node, s[i] - 'a');
        check(cur_node, flagman);
    }
}

std::vector<uint64_t> get_words_count(std::shared_ptr<TrieNode> root, std::vector<std::string> words, int number_of_patterns) {
    std::vector<uint64_t> counter(number_of_patterns, 0);
    for (auto& word : words) {
        std::vector<bool> flags(number_of_patterns, false);
        process_word(word, flags, root);
        for (int i = 0; i < number_of_patterns; ++i) {
            counter[i] += flags[i];
        }
    }
    return counter;
}

int main() {
    size_t n, m;
    std::vector<std::string> john_words, margaret_words;
    std::cin >> n;
    john_words.resize(n);
    std::cin.ignore();
    for (size_t i = 0; i < n; ++i) {
        std::getline(std::cin, john_words[i]);
    }
    std::cin >> m;
    std::cin.ignore();
    margaret_words.resize(m);
    for (size_t i = 0; i < m; ++i) {
        std::getline(std::cin, margaret_words[i]);
    }
    auto trie = create_trie(john_words);
    auto count = get_words_count(trie, margaret_words, n);
    for (uint64_t i : count) {
        std::cout << i << std::endl;
    }
    return 0;
}