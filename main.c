#include "music_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_help() {
    printf("\nAvailable Commands:\n");
    printf("  play              - Play current song\n");
    printf("  pause             - Pause / Resume\n");
    printf("  stop              - Stop playback\n");
    printf("  next              - Play next song\n");
    printf("  prev              - Play previous song\n");
    printf("  jump <n>          - Jump to song index n (1-based)\n");
    printf("  list              - Show playlist\n");
    printf("  info              - Show current song info\n");
    printf("  quit / exit       - Quit player\n");
    printf("  help              - Show this help menu\n\n");
}

int main(int argc, char *argv[]) {
    const char *playlist_path = "../playlist.json";
    if (argc > 1) {
        playlist_path = argv[1];
    }
    
    MusicPlayer *player = player_create(playlist_path);
    if (!player) {
        printf("Failed to create player.\n");
        return 1;
    }
    
    print_help();
    
    char input[256];
    while (1) {
        printf(">>> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;
        
        char cmd[64];
        int arg = 0;
        sscanf(input, "%s %d", cmd, &arg);
        
        if (strcmp(cmd, "play") == 0) {
            player_play_current(player);
        } else if (strcmp(cmd, "pause") == 0) {
            player_pause(player);
        } else if (strcmp(cmd, "stop") == 0) {
            player_stop(player);
        } else if (strcmp(cmd, "next") == 0) {
            player_next_track(player);
        } else if (strcmp(cmd, "prev") == 0 || strcmp(cmd, "previous") == 0) {
            player_previous_track(player);
        } else if (strcmp(cmd, "jump") == 0) {
            player_jump_to(player, arg);
        } else if (strcmp(cmd, "list") == 0) {
            player_show_playlist(player);
        } else if (strcmp(cmd, "info") == 0) {
            player_show_info(player);
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else {
            printf("Unknown command. Type 'help' for options.\n");
        }
        
        // Auto-next when song finishes (like Python version)
        if (player->playing && !player->paused && !player_is_playing(player)) {
            player_next_track(player);
        }
    }
    
    player_destroy(player);
    return 0;
}