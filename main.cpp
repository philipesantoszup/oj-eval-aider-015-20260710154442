#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>

using namespace std;

/**
 * Problem 015 - File Storage
 * Implementation: Disk-based Hash Table with Chaining.
 * 
 * Data Structure:
 * - A 'header' file (index.bin) stores offsets to the first entry of each bucket.
 * - A 'data' file (data.bin) stores the actual entries.
 * 
 * Entry Structure:
 * [Key (64 bytes)] [Value (4 bytes)] [NextOffset (8 bytes)]
 */

const string INDEX_FILE = "index.bin";
const string DATA_FILE = "data.bin";
const size_t BUCKET_COUNT = 100003; // Prime number for hash distribution
const size_t KEY_SIZE = 64;

struct Entry {
    char key[KEY_SIZE];
    int value;
    long long next_offset;
};

size_t hash_fn(const string& s) {
    size_t h = 0;
    for (char c : s) {
        h = h * 31 + c;
    }
    return h % BUCKET_COUNT;
}

class FileStorage {
    fstream index_fs;
    fstream data_fs;

public:
    FileStorage() {
        // Open index file: read/write, binary
        index_fs.open(INDEX_FILE, ios::in | ios::out | ios::binary);
        if (!index_fs) {
            index_fs.open(INDEX_FILE, ios::out | ios::binary);
            vector<long long> empty_buckets(BUCKET_COUNT, -1);
            index_fs.write(reinterpret_cast<char*>(empty_buckets.data()), BUCKET_COUNT * sizeof(long long));
            index_fs.close();
            index_fs.open(INDEX_FILE, ios::in | ios::out | ios::binary);
        }

        // Open data file: read/write, binary
        data_fs.open(DATA_FILE, ios::in | ios::out | ios::binary);
        if (!data_fs) {
            data_fs.open(DATA_FILE, ios::out | ios::binary);
            data_fs.close();
            data_fs.open(DATA_FILE, ios::in | ios::out | ios::binary);
        }
    }

    ~FileStorage() {
        if (index_fs.is_open()) index_fs.close();
        if (data_fs.is_open()) data_fs.close();
    }

    void insert(const string& key, int value) {
        size_t bucket = hash_fn(key);
        
        // Check if (key, value) already exists to prevent duplicates
        if (exists(key, value)) return;

        long long head_offset;
        index_fs.seekg(bucket * sizeof(long long));
        index_fs.read(reinterpret_cast<char*>(&head_offset), sizeof(long long));

        Entry new_entry;
        memset(new_entry.key, 0, KEY_SIZE);
        strncpy(new_entry.key, key.c_str(), KEY_SIZE - 1);
        new_entry.value = value;
        new_entry.next_offset = head_offset;

        // Append to end of data file
        data_fs.seekp(0, ios::end);
        long long new_offset = data_fs.tellp();
        data_fs.write(reinterpret_cast<char*>(&new_entry), sizeof(Entry));

        // Update index
        index_fs.seekp(bucket * sizeof(long long));
        index_fs.write(reinterpret_cast<char*>(&new_offset), sizeof(long long));
        index_fs.flush();
        data_fs.flush();
    }

    bool exists(const string& key, int value) {
        size_t bucket = hash_fn(key);
        long long current_offset;
        index_fs.seekg(bucket * sizeof(long long));
        index_fs.read(reinterpret_cast<char*>(&current_offset), sizeof(long long));

        while (current_offset != -1) {
            Entry e;
            data_fs.seekg(current_offset);
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (string(e.key) == key && e.value == value) return true;
            current_offset = e.next_offset;
        }
        return false;
    }

    void remove(const string& key, int value) {
        size_t bucket = hash_fn(key);
        long long current_offset;
        index_fs.seekg(bucket * sizeof(long long));
        index_fs.read(reinterpret_cast<char*>(&current_offset), sizeof(long long));

        long long prev_offset = -1;
        while (current_offset != -1) {
            Entry e;
            data_fs.seekg(current_offset);
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));

            if (string(e.key) == key && e.value == value) {
                if (prev_offset == -1) {
                    // Update head in index file
                    long long next = e.next_offset;
                    index_fs.seekp(bucket * sizeof(long long));
                    index_fs.write(reinterpret_cast<char*>(&next), sizeof(long long));
                } else {
                    // Update previous entry's next pointer
                    Entry prev_e;
                    data_fs.seekp(prev_offset);
                    data_fs.read(reinterpret_cast<char*>(&prev_e), sizeof(Entry));
                    prev_e.next_offset = e.next_offset;
                    data_fs.seekp(prev_offset);
                    data_fs.write(reinterpret_cast<char*>(&prev_e), sizeof(Entry));
                }
                index_fs.flush();
                data_fs.flush();
                return;
            }
            prev_offset = current_offset;
            current_offset = e.next_offset;
        }
    }

    void find(const string& key) {
        size_t bucket = hash_fn(key);
        long long current_offset;
        index_fs.seekg(bucket * sizeof(long long));
        index_fs.read(reinterpret_cast<char*>(&current_offset), sizeof(long long));

        vector<int> results;
        while (current_offset != -1) {
            Entry e;
            data_fs.seekg(current_offset);
            data_fs.read(reinterpret_cast<char*>(&e), sizeof(Entry));
            if (string(e.key) == key) {
                results.push_back(e.value);
            }
            current_offset = e.next_offset;
        }

        if (results.empty()) {
            cout << "null" << endl;
        } else {
            sort(results.begin(), results.end());
            for (size_t i = 0; i < results.size(); ++i) {
                cout << results[i] << (i == results.size() - 1 ? "" : " ");
            }
            cout << endl;
        }
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;

    FileStorage fs;
    string cmd, key;
    int val;

    for (int i = 0; i < n; ++i) {
        cin >> cmd;
        if (cmd == "insert") {
            cin >> key >> val;
            fs.insert(key, val);
        } else if (cmd == "delete") {
            cin >> key >> val;
            fs.remove(key, val);
        } else if (cmd == "find") {
            cin >> key;
            fs.find(key);
        }
    }

    return 0;
}
