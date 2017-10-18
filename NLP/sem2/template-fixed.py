from collections import Counter, defaultdict
import numpy as np

def train_hmm(tagged_sents):
    alpha=1e-24
    counter_tag = Counter()
    counter_tag_tag = Counter()
    counter_tag_word = Counter()
    tags = set()
    words = set()
    p_t_t = defaultdict(lambda: defaultdict(float))
    p_w_t = defaultdict(lambda: defaultdict(float))
    p_t = dict()
    for sent in tagged_sents:
        sent = [(word.lower(), tag) for word, tag in sent]
        sent_tags = [pair[1] for pair in sent]
        sent_words = [pair[0] for pair in sent]
        counter_tag.update(sent_tags)
        tags.update(sent_tags)
        counter_tag_tag.update([(ltag, rtag) for (_, ltag), (_, rtag) in zip(sent[:-1], sent[1:])])
        counter_tag_word.update(sent)
        words.update(sent_words)
    for (ltag, rtag), count in counter_tag_tag.items():
        p_t_t[ltag][rtag] = (count + alpha) / (counter_tag[ltag] + alpha * len(tags))
    for (word, tag), count in counter_tag_word.items():
        p_w_t[tag][word] = (count + alpha) / (counter_tag[tag] + alpha * (len(tags) + len(words)))
    for tag in tags:
        p_t[tag] = 1 / len(tags)
    return p_t, p_w_t, p_t_t


def viterbi_algorithm(test_tokens_list, p_t, p_w_t, p_t_t):
    delta = np.zeros([len(p_t), len(test_tokens_list)])
    s = np.zeros([len(p_t), len(test_tokens_list)], dtype='int64')
    states = list(p_t.keys())
    delta[:, 0] = np.array([np.log(p_t[state] or 10 ** -300) + np.log(p_w_t[state][test_tokens_list[0]] or 10 ** -300)
                            for state in states])
    for i in range(1, len(test_tokens_list)):
        for j in range(len(states)):
            values = [
                delta[k, i-1] + np.log(p_t_t[states[k]][states[j]] or 10 ** -300) + np.log(p_w_t[states[j]][test_tokens_list[i]] or 10 ** -300)
                for k in range(len(states))
            ]
            delta[j, i] = np.max(values)
            s[j, i] = np.argmax(values)
    result = [np.argmax(delta[:, -1])]
    for i in range(1, len(test_tokens_list)):
        result.append(s[result[-1], -i])
    return [states[i] for i in reversed(result)]
