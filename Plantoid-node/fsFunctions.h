// FILESYSTEM STUFF

#define FORMAT_SPIFFS_IF_FAILED true


String returnFile(fs::FS &fs, const char *path) {
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    if (serialDebug) Serial.println("- empty file or failed to open file");
    return String();
  }
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  return fileContent;
}




void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  if (serialDebug) Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    if (serialDebug) Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    if (serialDebug) Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (serialDebug) Serial.print("  DIR : ");
      if (serialDebug) Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      if (serialDebug) Serial.print("  FILE: ");
      if (serialDebug) Serial.print(file.name());
      if (serialDebug) Serial.print("\tSIZE: ");
      if (serialDebug) Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void readFile(fs::FS &fs, const char *path) {
  if (serialDebug) Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    if (serialDebug) Serial.println("- failed to open file for reading");
    return;
  }
  while (file.available()) {
    if (serialDebug) Serial.write(file.read());
  }
  file.close();
  if (serialDebug) Serial.println();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  if (serialDebug) Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    if (serialDebug) Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    if (serialDebug) Serial.println("- file written");
  } else {
    if (serialDebug) Serial.println("- write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  if (serialDebug) Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    if (serialDebug) Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    if (serialDebug) Serial.println("- message appended");
  } else {
    if (serialDebug) Serial.println("- append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  if (serialDebug) Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    if (serialDebug) Serial.println("- file renamed");
  } else {
    if (serialDebug) Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  if (serialDebug) Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    if (serialDebug) Serial.println("- file deleted");
  } else {
    if (serialDebug) Serial.println("- delete failed");
  }
}