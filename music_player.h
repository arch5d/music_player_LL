#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Song structure
typedef struct {
    int id;
    char *title;
    char *filename;
    char *b64_data;
} Song;

// Doubly Linked List Node
typedef struct Node {
    Song *song;
    struct Node *prev;
    struct Node *next;
} Node;

// Doubly Linked List
typedef struct {
    Node *head;
    Node *tail;
    int length;
} DoublyLinkedList;

// Music Player
typedef struct {
    DoublyLinkedList *playlist;
    Node *current_node;
    char *current_tempfile;
    int playing;
    int paused;
} MusicPlayer;

// Linked List functions
DoublyLinkedList* dll_create();
Node* dll_append(DoublyLinkedList *dll, Song *song);
Node* dll_get_at_index(DoublyLinkedList *dll, int index);
void dll_destroy(DoublyLinkedList *dll);

// Song functions
Song* song_create(int id, const char *title, const char *filename, const char *b64);
void song_destroy(Song *song);

// Music Player functions
MusicPlayer* player_create(const char *playlist_path);
void player_destroy(MusicPlayer *player);
int player_load_playlist(MusicPlayer *player, const char *path);
void player_play_current(MusicPlayer *player);
void player_stop(MusicPlayer *player);
void player_pause(MusicPlayer *player);
void player_next_track(MusicPlayer *player);
void player_previous_track(MusicPlayer *player);
void player_jump_to(MusicPlayer *player, int index);
void player_show_playlist(MusicPlayer *player);
void player_show_info(MusicPlayer *player);
int player_is_playing(MusicPlayer *player);

// Utility functions
char* base64_decode(const char *input, size_t *out_len);
char* read_file(const char *path);
int parse_json_playlist(const char *json_str, DoublyLinkedList *dll);

#endif
