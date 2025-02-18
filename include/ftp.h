

/// @brief Sets up the FTP state to be able to start a download. Opens the connections, logs in and activates passive mode.
/// @param url a pointer to the FTP url, which contains the host to download from and name of the file to download, and optional user and password information  
/// @return 0 on success, 1 on failure
int ftp_init(char * url);
/// @brief Downloads the file: sends the RETR and reads from the data connection.
/// @param filename The name the file should be stored as
/// @return 0 on success, 1 on failure
int ftp_download_file(char *filename);
/// @brief Closes the FTP connection: sends the QUIT command and closes the socket.
/// @return 0 on success, 1 on failure
int ftp_close();