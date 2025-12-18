# Standard Library

Sunda comes with built-in modules for common tasks.

## Global Functions

- `print(args...)`: Prints values without a newline.
- `println(args...)`: Prints values with a newline.

## String Module
```javascript
import { length, substr, contains } from "string";

var s = "Hello Sunda";
println(s.length); // 11
```

## Array Module
```javascript
var list = [1, 2, 3];
list.push(4);
list.pop();

var doubled = list.map(n => n * 2);
var evens = list.filter(n => n % 2 == 0);
```

## Map Module
```javascript
var user = { id: 1 };
user.name = "Alice";
println(user.name);
```
### `db` Module
The `db` module provides a unified interface for database operations.

- `db_connect(url)`: Connects to a database. Supports `sqlite://`, `mysql://`, and `mariadb://`.
- `db_query(sql, params)`: Executes a query and returns result as an array of objects.
- `db_execute(sql, params)`: Executes a command (insert, update, delete).
- `db_close()`: Closes the active connection.
- `db_error()`: Returns the last error message.

Example (SQL Injection Protected):
```javascript
import { db_connect, db_execute, db_query } from "db";
db_connect("sqlite://examples/test_db/test.db");
db_execute("INSERT INTO users (name) VALUES (?)", ["Yudono"]);
const users = db_query("SELECT * FROM users");
```

### `json` Module
The `json` module provides utilities for parsing and serializing JSON data.

- `json_parse(string)`: Parses a JSON string and returns it as a Sunda `Value` (map, array, number, string, or boolean).

Example:
```javascript
import { json_parse } from "json";
const obj = json_parse('{"name": "Sunda", "stable": true}');
println(obj.name); // Sunda
```

### `webserver` Module
The `webserver` module provides a low-level TCP/IP server with a high-level API.

- `WebServer_create()`: Creates a new server instance.
  - `app.get(path, handler)`: Registers a GET route.
  - `app.post(path, handler)`: Registers a POST route.
  - `app.put(path, handler)`: Registers a PUT route.
  - `app.delete(path, handler)`: Registers a DELETE route.
  - `app.patch(path, handler)`: Registers a PATCH route.
  - `app.listen({ port })`: Starts the server.
- `c.text(body)`: Send plain text response.
- `c.html(body)`: Send HTML response.
- `c.json(obj)`: Send JSON response.
- `c.response(type, body)`: Generic response helper.
- `c.req.param(name)`: URL parameter.
- `c.req.body`: Raw request body.
- `c.req.json()`: Automatically parses JSON body.
- `c.req.header(name)`: Access request header.

### `fs` Module
File system operations.
- `fs_readFile(path)`: Returns file content as string.
- `fs_writeFile(path, content)`: Writes content to file.
- `fs_exists(path)`: Checks if file exists.
- `fs_listDir(path)`: Returns array of filenames in directory.
- `fs_mkdir(path)`: Creates directory recursively.
- `fs_remove(path)`: Deletes file or directory.

### `os` Module
Operating system and environment.
- `os_getenv(name)`: Returns environment variable.
- `os_platform()`: Returns "macos", "linux", or "windows".
- `os_cwd()`: Returns current working directory.

### `exec` Module
Subprocess execution.
- `exec_run(command)`: Runs command and returns stdout as string.

### `regex` Module
Regular expression operations.
- `regex_match(str, pattern)`: Boolean match.
- `regex_search(str, pattern)`: Boolean search.
- `regex_replace(str, pattern, replacement)`: Returns replaced string.

## String Module
```javascript
import { split, join, replace, concat, find, substring } from "string";

- `concat(a, b, ...)`: Concatenate multiple values into a string.
- `find(str, search)`: Alias for `indexOf`. Returns the position of the first occurrence.
- `substring(str, start, end)`: Extracts a part of a string.
- `str_length(str)`: Returns the length of the string.

```javascript
import { concat, find, substring } from "string";
const full = concat("Sunda", " ", "Runtime");
const pos = find(full, "Runtime"); // 6
```

## Math Module
```javascript
import { sin, cos, random, floor } from "math";

println(floor(3.14)); // 3
```

## Date Module
```javascript
import { now, format } from "date";

println(now()); // Current timestamp
```
