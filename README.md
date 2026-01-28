This is an SQL-based relational database management system (DBMS) that is still a work in progress.

The plan is to have an SQL interpreter that transforms SQL queries into bytecode which will run on a VM. The bytecode will control how the DBMS manages the user's data, which is stored on an esoteric variation of a binary search tree called a B+ tree. The DBMS will work with variable length data and will stage transactions before changing data per ACID-compliance.

Additionally, I'm planning to add a mode to this program that will create a database that is self populated with a user's local file storage data, hence FileSQLite. I feel like it would be cool to gain a more comprehensive understanding of my own file storage via the capabilities of SQL.

As an added challenge, and because I love creating systems from scratch, this project is created using only the C Standard Library.