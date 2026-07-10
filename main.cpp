#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>

using namespace std;

/**
 * Problem 015 - File Storage
 * Implementation: Disk-based Hash Table with Chaining.
 * 
 * Optimization:
 * - The index (bucket heads) is cached in memory to avoid frequent disk I/O.
 * - Data entries remain on disk.
 */

const char* INDEX_FILE = "index.bin";
const char* DATA_FILE = "data.bin";
const size_t BUCKET_COUNT = 100003; 
const size_t KEY_SIZE = 64;

struct Entry {
    char key[KEY_SIZE];
    int value;
    long long next_offset;
};

size_t hash_fn(const string& s) {
    size_t h = 0;
    for (char c : s) {
        h = h * 31 + (unsigned char)c;
    }
    return h % BUCKET_COUNT;
}

class FileStorage {
    FILE* index_fp;
    FILE* data_fp;
    vector<long long> index_cache;

public:
    FileStorage() {
        index_cache.assign(BUCKET_COUNT, -1);
        
        // Open index file
        index_fp = fopen(INDEX_FILE, "rb+");
        if (!index_fp) {
            index_fp = fopen(INDEX_FILE, "wb+");
            vector<long long> empty_buckets(BUCKET_COUNT, -1);
            fwrite(empty_buckets.data(), sizeof(long long), BUCKET_COUNT, index_fp);
            fflush(index_fp);
        } else {
            // Load index into memory
            fread(index_cache.data(), sizeof(long long), BUCKET_COUNT, index_fp);
        }

        // Open data file
        data_fp = fopen(DATA_FILE, "rb+");
        if (!data_fp) {
            data_fp = fopen(DATA_FILE, "wb+");
        }
    }

    ~FileStorage() {
        if (index_fp) {
            // Sync index cache to disk before closing
            fseek(index_fp, 0, SEEK_SET);
            fwrite(index_cache.data(), sizeof(long long), BUCKET_COUNT, index_fp);
            fclose(index_fp);
        }
        if (data_fp) fclose(data_fp);
    }

    void insert(const string& key, int value) {
        size_t bucket = hash_fn(key);
        long long head_offset = index_cache[bucket];

        // Check for duplicates while traversing
        long long current_offset = head_offset;
        while (current_offset != -1) {
            Entry e;
            fseek(data_fp, current_offset, SEEK_SET);
            fread(&e, sizeof(Entry), 1, data_fp);
            if (strncmp(e.key, key.c_str(), KEY_SIZE) == 0 && e.value == value) return;
            current_offset = e.next_offset;
        }

        Entry new_entry;
        memset(new_entry.key, 0, KEY_SIZE);
        strncpy(new_entry.key, key.c_str(), KEY_SIZE - 1);
        new_entry.value = value;
        new_entry.next_offset = head_offset;

        // Append to end of data file
        fseek(data_fp, 0, SEEK_END);
        long long new_offset = ftell(data_fp);
        fwrite(&new_entry, sizeof(Entry), 1, data_fp);

        // Update index in memory
        index_cache[bucket] = new_offset;
        
        // Periodically sync index to disk to be safe, or just rely on destructor.
        // For OJ, syncing the specific bucket is fast.
        fseek(index_fp, bucket * sizeof(long long), SEEK_SET);
        fwrite(&index_cache[bucket], sizeof(long long), 1, index_fp);
    }

    void remove(const string& key, int value) {
        size_t bucket = hash_fn(key);
        long long current_offset = index_cache[bucket];

        long long prev_offset = -1;
        while (current_offset != -1) {
            Entry e;
            fseek(data_fp, current_offset, SEEK_SET);
            fread(&e, sizeof(Entry), 1, data_fp);

            if (strncmp(e.key, key.c_str(), KEY_SIZE) == 0 && e.value == value) {
                if (prev_offset == -1) {
                    long long next = e.next_offset;
                    index_cache[bucket] = next;
                    fseek(index_fp, bucket * sizeof(long long), SEEK_SET);
                    fwrite(&index_cache[bucket], sizeof(long long), 1, index_fp);
                } else {
                    Entry prev_e;
                    fseek(data_fp, prev_offset, SEEK_SET);
                    fread(&prev_e, sizeof(Entry), 1, data_fp);
                    prev_e.next_offset = e.next_offset;
                    fseek(data_fp, prev_offset, SEEK_SET);
                    fwrite(&prev_e, sizeof(Entry), 1, data_fp);
                }
                return;
            }
            prev_offset = current_offset;
            current_offset = e.next_offset;
        }
    }

    void find(const string& key) {
        size_t bucket = hash_fn(key);
        long long current_offset = index_cache[bucket];

        vector<int> results;
        while (current_offset != -1) {
            Entry e;
            fseek(data_fp, current_offset, SEEK_SET);
            fread(&e, sizeof(Entry), 1, data_fp);
            if (strncmp(e.key, key.c_str(), KEY_SIZE) == 0) {
                results.push_back(e.value);
            }
            current_offset = e.next_offset;
        }

        if (results.empty()) {
            printf("null\n");
        } else {
            sort(results.begin(), results.end());
            for (size_t i = 0; i < results.size(); ++i) {
                printf("%d%c", results[i], (i == results.size() - 1 ? '\n' : ' '));
            }
        }
    }
};

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    FileStorage fs;
    char cmd[20], key[100];
    int val;

    for (int i = 0; i < n; ++i) {
        if (scanf("%s", cmd) != 1) break;
        string cmd_str(cmd);
        if (cmd_str == "insert") {
            scanf("%s %d", key, &val);
            fs.insert(key, val);
        } else if (cmd_str == "delete") {
            scanf("%s %d", key, &val);
            fs.remove(key, val);
        } else if (cmd_str == "find") {
            scanf("%s", key);
            fs.find(key);
        }
    }

    return 0;
}
