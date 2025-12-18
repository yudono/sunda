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
