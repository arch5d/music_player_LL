#include "music_player.h"
#include <ctype.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// Base64 decoding table
static const unsigned char base64_table[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

char* base64_decode(const char *input, size_t *out_len) {
    size_t len = strlen(input);
    size_t output_len = len / 4 * 3;
    if (len > 0 && input[len - 1] == '=') output_len--;
    if (len > 1 && input[len - 2] == '=') output_len--;
    
    unsigned char *output = malloc(output_len);
    if (!output) return NULL;
    
    size_t j = 0;
    for (size_t i = 0; i < len; i += 4) {
        unsigned char a = base64_table[(unsigned char)input[i]];
        unsigned char b = base64_table[(unsigned char)input[i + 1]];
        unsigned char c = base64_table[(unsigned char)input[i + 2]];
        unsigned char d = base64_table[(unsigned char)input[i + 3]];
        
        output[j++] = (a << 2) | (b >> 4);
        if (c != 64) output[j++] = (b << 4) | (c >> 2);
        if (d != 64) output[j++] = (c << 6) | d;
    }
    
    *out_len = output_len;
    return (char*)output;
}

// Simple JSON parser (minimal, specific to our format)
char* read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = 0;
    fclose(f);
    return content;
}

static char* extract_string(const char *json, const char *key) {
    char search[256];
    sprintf(search, "\"%s\":", key);
    const char *pos = strstr(json, search);
    if (!pos) return NULL;
    
    // Move past the key and colon
    pos += strlen(search);
    // Skip whitespace
    while (*pos && isspace(*pos)) pos++;
    // Now we should be at the opening quote of the value
    if (*pos != '"') return NULL;
    pos++; // skip opening quote
    
    const char *end = pos;
    while (*end && *end != '"') {
        if (*end == '\\') end++; // skip escaped chars
        end++;
    }
    
    size_t len = end - pos;
    char *result = malloc(len + 1);
    strncpy(result, pos, len);
    result[len] = 0;
    return result;
}

static int extract_int(const char *json, const char *key) {
    char search[256];
    sprintf(search, "\"%s\":", key);
    const char *pos = strstr(json, search);
    if (!pos) return 0;
    
    pos += strlen(search);
    while (*pos && isspace(*pos)) pos++;
    return atoi(pos);
}

int player_is_playing(MusicPlayer *player) {
    (void)player; // unused
#ifdef _WIN32
    char status[64] = {0};
    mciSendStringA("status mp3 mode", status, sizeof(status), NULL);
    return strcmp(status, "playing") == 0;
#else
    return 0; // simplified for Unix
#endif
}

int parse_json_playlist(const char *json_str, DoublyLinkedList *dll) {
    const char *songs_start = strstr(json_str, "\"songs\":");
    if (!songs_start) return 0;
    
    songs_start = strchr(songs_start, '[');
    if (!songs_start) return 0;
    songs_start++;
    
    const char *song_start = songs_start;
    int count = 0;
    
    while ((song_start = strchr(song_start, '{')) != NULL) {
        const char *song_end = strchr(song_start, '}');
        if (!song_end) break;
        
        size_t song_len = song_end - song_start + 1;
        char *song_json = malloc(song_len + 1);
        strncpy(song_json, song_start, song_len);
        song_json[song_len] = 0;
        
        int id = extract_int(song_json, "id");
        char *title = extract_string(song_json, "title");
        char *filename = extract_string(song_json, "filename");
        char *b64 = extract_string(song_json, "b64");
        
        if (title && filename && b64) {
            Song *song = song_create(id, title, filename, b64);
            dll_append(dll, song);
            count++;
        }
        
        free(title);
        free(filename);
        free(b64);
        free(song_json);
        
        song_start = song_end + 1;
    }
    
    return count;
}

// Song functions
Song* song_create(int id, const char *title, const char *filename, const char *b64) {
    Song *song = malloc(sizeof(Song));
    song->id = id;
    song->title = strdup(title);
    song->filename = strdup(filename);
    song->b64_data = strdup(b64);
    return song;
}

void song_destroy(Song *song) {
    if (song) {
        free(song->title);
        free(song->filename);
        free(song->b64_data);
        free(song);
    }
}

// Doubly Linked List functions
DoublyLinkedList* dll_create() {
    DoublyLinkedList *dll = malloc(sizeof(DoublyLinkedList));
    dll->head = NULL;
    dll->tail = NULL;
    dll->length = 0;
    return dll;
}

Node* dll_append(DoublyLinkedList *dll, Song *song) {
    Node *node = malloc(sizeof(Node));
    node->song = song;
    node->prev = NULL;
    node->next = NULL;
    
    if (!dll->head) {
        dll->head = dll->tail = node;
    } else {
        node->prev = dll->tail;
        dll->tail->next = node;
        dll->tail = node;
    }
    dll->length++;
    return node;
}

Node* dll_get_at_index(DoublyLinkedList *dll, int index) {
    if (index < 1 || index > dll->length) return NULL;
    
    Node *cur = dll->head;
    for (int i = 1; i < index && cur; i++) {
        cur = cur->next;
    }
    return cur;
}

void dll_destroy(DoublyLinkedList *dll) {
    if (!dll) return;
    Node *cur = dll->head;
    while (cur) {
        Node *next = cur->next;
        song_destroy(cur->song);
        free(cur);
        cur = next;
    }
    free(dll);
}

// Audio playback functions (Windows MCI)
#ifdef _WIN32
// MCI helpers
static int mci_ok(const char *cmd) {
    MCIERROR e = mciSendStringA(cmd, NULL, 0, NULL);
    if (e != 0) {
        return 0;
    }
    return 1;
}

static void stop_mp3() {
    mci_ok("stop mp3");
    mci_ok("close mp3");
}

static int play_mp3_file(const char *filename) {
    char full[MAX_PATH] = {0};
    if (!GetFullPathNameA(filename, MAX_PATH, full, NULL) || !full[0]) {
        return 0;
    }

    mci_ok("close mp3");

    char cmd[1024];
    sprintf(cmd, "open \"%s\" alias mp3", full);
    if (!mci_ok(cmd)) {
        sprintf(cmd, "open \"%s\" type mpegvideo alias mp3", full);
        if (!mci_ok(cmd)) {
            return 0;
        }
    }

    if (!mci_ok("play mp3 from 0")) {
        mci_ok("close mp3");
        return 0;
    }
    return 1;
}

static void pause_mp3() {
    mci_ok("pause mp3");
}

static void resume_mp3() {
    mci_ok("resume mp3");
}
#else
// Unix/Linux - use system command
static void play_mp3_file(const char *filename) {
    char cmd[512];
    sprintf(cmd, "mpg123 -q \"%s\" &", filename);
    system(cmd);
}

static void stop_mp3() {
    system("killall mpg123 2>/dev/null");
}

static void pause_mp3() {
    system("killall -STOP mpg123 2>/dev/null");
}

static void resume_mp3() {
    system("killall -CONT mpg123 2>/dev/null");
}
#endif

// Music Player functions
MusicPlayer* player_create(const char *playlist_path) {
    MusicPlayer *player = malloc(sizeof(MusicPlayer));
    player->playlist = dll_create();
    player->current_node = NULL;
    player->current_tempfile = NULL;
    player->playing = 0;
    player->paused = 0;
    
    if (!player_load_playlist(player, playlist_path)) {
        player_destroy(player);
        return NULL;
    }
    
    player->current_node = player->playlist->head;
    return player;
}

int player_load_playlist(MusicPlayer *player, const char *path) {
    char *json_str = read_file(path);
    if (!json_str) {
        printf("Playlist not found: %s\n", path);
        return 0;
    }
    
    int count = parse_json_playlist(json_str, player->playlist);
    free(json_str);
    
    if (count == 0) {
        printf("No songs found in playlist.\n");
        return 0;
    }
    
    printf("Loaded %d songs into playlist.\n\n", count);
    return 1;
}

void player_play_current(MusicPlayer *player) {
    if (!player->current_node) {
        printf("No song selected.\n");
        return;
    }
    
    Song *song = player->current_node->song;
    
    size_t decoded_len;
    char *decoded = base64_decode(song->b64_data, &decoded_len);
    if (!decoded) {
        printf("Error decoding song.\n");
        return;
    }
    
    if (player->current_tempfile) {
        remove(player->current_tempfile);
        free(player->current_tempfile);
    }
    
    player->current_tempfile = strdup("temp_song.mp3");
    FILE *f = fopen(player->current_tempfile, "wb");
    if (!f) {
        printf("Error creating temp file.\n");
        free(decoded);
        return;
    }
    fwrite(decoded, 1, decoded_len, f);
    fclose(f);
    free(decoded);

    if (player->playing) {
        stop_mp3();
    }

#ifdef _WIN32
    if (!play_mp3_file(player->current_tempfile)) {
        printf("Failed to play song.\n");
        return;
    }
#else
    play_mp3_file(player->current_tempfile);
#endif
    player->playing = 1;
    player->paused = 0;
    printf("Now playing: [%d] %s\n", song->id, song->title);
}

void player_stop(MusicPlayer *player) {
    stop_mp3();
    player->playing = 0;
    player->paused = 0;
    printf("Stopped playback.\n");
}

void player_pause(MusicPlayer *player) {
    if (!player->playing) {
        printf("Nothing is playing.\n");
        return;
    }
    
    if (player->paused) {
        resume_mp3();
        player->paused = 0;
        printf("Resumed.\n");
    } else {
        pause_mp3();
        player->paused = 1;
        printf("Paused.\n");
    }
}

void player_next_track(MusicPlayer *player) {
    if (!player->current_node) {
        printf("No current song.\n");
        return;
    }
    
    if (!player->current_node->next) {
        printf("Reached end of playlist.\n");
        player_stop(player);
        return;
    }
    
    player->current_node = player->current_node->next;
    printf("Next: %s\n", player->current_node->song->title);
    player_play_current(player);
}

void player_previous_track(MusicPlayer *player) {
    if (!player->current_node) {
        printf("No current song.\n");
        return;
    }
    
    if (!player->current_node->prev) {
        printf("Already at first song.\n");
        return;
    }
    
    player->current_node = player->current_node->prev;
    printf("Previous: %s\n", player->current_node->song->title);
    player_play_current(player);
}

void player_jump_to(MusicPlayer *player, int index) {
    Node *node = dll_get_at_index(player->playlist, index);
    if (!node) {
        printf("Invalid song index.\n");
        return;
    }
    
    player->current_node = node;
    printf("Jumped to: %s\n", node->song->title);
    player_play_current(player);
}

void player_show_playlist(MusicPlayer *player) {
    printf("\nPlaylist:\n");
    Node *cur = player->playlist->head;
    int i = 1;
    while (cur) {
        const char *prefix = (cur == player->current_node) ? "->" : "  ";
        printf("%s [%d] %s\n", prefix, i, cur->song->title);
        cur = cur->next;
        i++;
    }
    printf("\n");
}

void player_show_info(MusicPlayer *player) {
    if (player->current_node) {
        Song *s = player->current_node->song;
        printf("Now: [%d] %s (%s)\n", s->id, s->title, s->filename);
    } else {
        printf("No current song.\n");
    }
}

void player_destroy(MusicPlayer *player) {
    if (!player) return;
    
    stop_mp3();
    if (player->current_tempfile) {
        remove(player->current_tempfile);
        free(player->current_tempfile);
    }
    
    dll_destroy(player->playlist);
    free(player);
}