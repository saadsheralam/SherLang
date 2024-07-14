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

SherLang> def {y z} 10 20 # declare multiple variables 
ok

SherLang> def {x} "Thanks for using SherLang" # declare a string
ok

SherLang> def {my_list} {1 2 3 4} # declare a list 
ok
```

```
# Arithmetic 

SherLang> + 1 (* 7 5) 3
39.00

SherLang> def {x y} 10 20 
ok 

SherLang> - x 
-10.00 

SherLang> % 20 (/ x (+ x y))
0.00 
```

```
# Builtin Functions

SherLang> list 1 2 3 4
{1.00 2.00 3.00 4.00}

SherLang> eval {head (list 1 2 3 4)}
{1.00}

SherLang> tail {tail tail tail} 
{tail tail}

SherLang> eval (head {(+ 1 2) (+ 10 20)})
3.00

SherLang> (eval (head {+ - + - * /})) 10 20 
30.00

SherLang> head {1 2 3} {4 5 6}
Error: Function 'head' passed incorrect number of arguments. Got 2, Expected 1.

SherLang> error "This is an error!"
Error: This is an error!

SherLang> len {1 2 3 4} 
4.00 
SherLang> len ""
0.0

SherLang> join {1 2 3 4} {5 6 7 8} {9 10 11 12}
{1.00 2.00 3.00 4.00 5.00 6.00 7.00 8.00 9.00 10.00 11.00 12.00}

SherLang> join "This" "is" "SherLang"
"ThisisSherLang"

SherLang> cons 2 {1 2 3}
{2.00 1.00 2.00 3.00}


```


```
# Defining Functions 

SherLang> fun {add-together x y} {+ x y} 
ok 

SherLang> add-together 10 20 
30.00 

SherLang> (fun {my_len l} {  if (== l {})    {0}    {+ 1 (my_len (tail l))}})
ok 

SherLang> my_len {1 2 3 4 5}
5.00 

SherLang> (fun {reverse l} {  if (== l {})    {{}}    {join (reverse (tail l)) (head l)}}) 
ok 

SherLang> reverse {1 2 3 4 5}
{5.00 4.00 3.00 2.00 1.00}


```
