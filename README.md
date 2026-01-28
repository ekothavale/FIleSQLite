This is an ACID-compliant SQL-based relational database management system (DBMS) that is still a work in progress.

The plan is to have an SQL interpreter that transforms SQL queries into bytecode which will run on a VM. The bytecode will control how the DBMS manages the user's data, which is stored on an esoteric variation of a binary search tree called a B+ tree.