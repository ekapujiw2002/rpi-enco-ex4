/* get active pls and play it if any */
while (1) {
    /* get active pls id if any */
    active_pls_id = find_current_time_pls(playlist_total_item, playlist_controls);

    /* got any? */
    if ((active_pls_id >= 0) && (active_pls_id < playlist_total_item)) {
        /* process the playlist */
        flag_playlist_exit_result = playlist_process_active(playlist_controls[active_pls_id], playlist_items, playlist_audio_items);

        /* info */
        printf(KGRN "Playlist #%d done (%d)\n" RESET, active_pls_id, flag_playlist_exit_result);

        /* action for exit code. break directly for code 3 (new pls found), 1, and 2 for special case */
        if (flag_playlist_exit_result != 2)
            break;
        else {
            if (DATE_OLD != strToTime("00:00:00"))
                break;
        }
    } else {
        /* got no active playlist */
        printf(KRED "Got no active playlist to be played\n" RESET);
        break;
    }
}

while (1) {
    /* get active pls id if any */
    active_pls_id = find_current_time_pls(playlist_total_item, playlist_controls);

    /* got any? */
    if ((active_pls_id >= 0) && (active_pls_id < playlist_total_item)) {
        /* process the playlist */
        flag_playlist_exit_result = playlist_process_active(playlist_controls[active_pls_id], playlist_items, playlist_audio_items);

        /* info */
        printf(KGRN "Playlist #%d done (%d)\n" RESET, active_pls_id, flag_playlist_exit_result);

        /* break */
        if ((flag_playlist_exit_result == 1) || (flag_playlist_exit_result == 3)) {
            if (flag_playlist_exit_result == 1)
                DATE_OLD = strToTime("00:00:00");
            break;
        }
    } else {
        /* got no active playlist */
        printf(KRED "Got no active playlist to be played\n" RESET);
        break;
    }
}
