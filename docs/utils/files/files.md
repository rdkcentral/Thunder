The `Core::File` API can be used for common filesystem tasks.

Below are some examples of using the File API, refer to `Source/core/Filesystem.h` and `Source/core/Filesystem.cpp` for all available APIs.

## File Handling

### Check if file exists

```cpp
if (Core::File("/tmp/test.txt").Exists()) {
	printf("File exists\n");
} else {
	printf("File does not exist\n");
}
```

### Create new file

```cpp
Core::File fileObject("/tmp/test.txt");
fileObject.Create(static_cast<uint32_t>(0755));

// Either close the file manually, or it will be closed automatically in destructor
fileObject.Close();
```

### Delete File

```cpp
Core::File fileObject("/tmp/test.txt");
file.Destroy();
```

### Move File

```cpp
Core::File fileObject("/tmp/test.txt");
fileObject.Move("/tmp/newfile.txt");

// The fileObject information is refreshed once moved, so calls to
// methods such as `Name()` will contain the updated values from after the move.
printf("New file path is: %s\n", fileObject.Name().c_str());
```

### Get File Extension

Will return file extension without the leading dot

```cpp
// Option 1
string extension = Core::File::Extension("/tmp/test.txt");

// Option 2
Core::File fileObject("/tmp/test.txt");
string extension = fileObject.Extension();
```

### Read file contents into memory

!!! warning
	Consider memory usage when reading large files straight into memory. For large files, consider reading and parsing line-by-line to extract only the necessary data if possible

```cpp
Core::File fileObject("/tmp/test.txt");

// Open() defaults to read-only
if (fileObject.Open()) {
    // Successfully opened the file, now read into a buffer
    char buffer[1024] = {};
    uint32_t read;
    string fileContents;

    while ((read = fileObject.Read(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer))) != 0) {
        fileContents.append(buffer, read);
    }

    fileObject.Close();
}
```

### Create a new file and write data

```cpp
Core::File fileObject("/tmp/test.txt");

// Delete file if it already exists
if (fileObject.Exists()) {
    fileObject.Destroy();
}

fileObject.Create();

string toWrite = "Some text to write to a file\n";
fileObject.Write(reinterpret_cast<uint8_t*>(toWrite.data()), toWrite.size());

fileObject.Close();
```

### Append to existing file

```cpp
Core::File fileObject("/tmp/test.txt");

// Open() defaults to opening read-only, pass false to open as r/w
fileObject.Open(false);
fileObject.Position(false, fileObject.Size());

string toWrite = "Text to append to a file\n";
fileObject.Write(reinterpret_cast<uint8_t*>(toWrite.data()), toWrite.size());

fileObject.Close();
```

### Read file line-by-line

This is a more efficient way of parsing large files instead of reading them entirely into memory if only certain information in the file is required

```cpp
// File will be closed on destruction
Core::DataElementFile fileObject("/tmp/test.txt", Core::File::USER_READ);
Core::TextReader reader(fileObject);

while (!reader.EndOfText()) {
    Core::TextFragment line(reader.ReadLine());

    if (!line.IsEmpty()) {
        printf("Read line: %s\n", line.Text().c_str());
    }
}
```

## Directory Handling

### Create Directory

Will create directories recursively

```cpp
Core::Directory("/tmp/subdirectory").CreatePath();
```

### Delete contents of a directory

This will remove everything inside the directory recursively, but will not delete the directory itself

```cpp
Core::Directory("/tmp/subdirectory").Destroy();
```

### Delete directory

This will delete the contents of the directory and the directory itself

```
Core::Directory("/tmp/subdirectory").Destroy();
Core::File("/tmp/subdirectory").Destroy();
```

### Iterate over a directory

#### All Files

```cpp
Core::Directory dir("/tmp");

while (dir.Next()) {
    Core::File currentEntry(dir.Current());

    if (!currentEntry.IsDirectory()) {
        printf("Found file: %s\n", currentEntry.Name().c_str());
    }
}
```

#### Globbing

Using an glob wildcard pattern, only iterate over .txt files

``` cpp
Core::Directory dir("/tmp", "*.txt");

while (dir.Next()) {
    Core::File currentEntry(dir.Current());

    if (!currentEntry.IsDirectory()) {
        printf("Found text file: %s\n", currentEntry.Name().c_str());
    }
}
```

## Path Handling

### Normalisation

For security, use the `Normalise()` function when accepting user input to prevent path traversal vulnerabilities. Will attempt to return a safe version of the path if possible. If the path attempts to traverse outside of the current directory then the `valid` argument will be set to false.

```cpp
std::string vulnerablePath = "../../../../passwd";
bool validPath;

Core::File::Normalize(vulnerablePath, validPath);

printf("Safe path: %s\n", validPath ? "true" : "false");

/* Output:
Safe path: false
*/
```



