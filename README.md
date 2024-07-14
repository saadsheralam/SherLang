# SherLang 
SherLang is a minimal and lightweight LISP-like programming language implemented in C. The language has it's own handwritten interpreter. 

### Features 
SherLang supports the following features: 
- [x] Data Types: Integer, Floating Point, Boolean, Char, String 
- [x] Builtin Data Structures: List (Ofcourse, its a LISP :p)
- [x] Polish Arithmetic Notation 
- [x] Ordering and Logical Operations: >, <, >=, <=, !=, &&, ||, !
- [x] Functions, Recursive Functions, and Lamba Functions
- [x] Builtin Functions:  *head*, *tail*, *join*, *len*, *list*, *cons*, *def*, *put*, *lambda*, *load*, *print*, *error* 
- [x] A Standard Library 

The language is purposefully kept simple and lightweight. The implemented constructs are chosen carefully such that these constructs can be easily used to implement any new complex feature. 

### Documentation and Usage 

```
# Defining variables 
SherLang> def {x} 100 # declare a single variable
ok
SherLang> def {y,z} 10,20 # declare multiple variables 
ok
SherLang> def {x} "Thanks for using SherLang" # declare a string
ok
SherLang> def {my_list} {1 2 3 4} # declare a list 
ok
```
