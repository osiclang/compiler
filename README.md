
<img src="https://github.com/OSIClang/compiler/blob/master/logo/osic-owl-text.png" width="200">
OSIC Compiler is a compiler for the OSIC language. OSIC stands for Objectiv Symbolic Instruction Code and should be completely compilable to a wide spectrum of machines in addition to a programming language that is very easy to learn.

### Docs

OSIC Documentations:

* [Docs](commands.md) - How to write code with OSIC language 
* `src` source code of OSIC compiler and interpreter
* `lib` source code of core OSIC library

### How to Use

This Compiler/Interpreter requires GCC.

```sh
$ apt-get install build-essential
```

To test the Compiler (this is far away from productive Use).

```sh
$ osic example.osic
```

Enter interpreter "live mode"

```sh
$ ./osic
```

To exit the "live mode" just insert `\exit`



### Build Instructions
```
make
```
or

```
make DEBUG=0 STATIC=0 USE_MALLOC=0 MODULE_OS=1 MODULE_SOCKET=1
```

* `DEBUG`, debug compiler flags as boolean- in this case `false`
* `STATIC`, 0 build with dynamic-linked library, 1 build with static-linked.
* `USE_MALLOC`, stdlib's `malloc` ensure return aligned pointer
* `MODULE_OS`, for POSIX builtin os library is true
* `MODULE_SOCKET`, for BSD Socket builtin library is true

Windows Platform
----------------

Lemon can build on Windows via [Mingw](http://www.mingw.org/wiki/Install_MinGW),
getting source code and use command

```
mingw32-make
```

### Porting

`lib/os.c` and `lib/socket.c` are supporting POSIX and Microsoft Windows environment. Ensure you have a compatible Windows Build environment installed.

# Roadmap
- :heavy_check_mark: Add print and input
- :heavy_check_mark: Add handling for methods
- :x: Add Writer commands to write files and send via network
- :x: Add Reader to read files and recieve request on ports
- :x: The language must pass the touring test
- :heavy_check_mark: Remove eclipse depencies - it should use his own code parser
- :x: Add Packagehandling
- :x: Add ability to work with classes and new instances of it
- :x: final: Rewrite Code to make him compile himself!

# Add your Review (a little checklist)
- Make sure that methods of styling are similar to already existing related methods. Look at POJO's and helpers. This ensures that the code has a readable style.
- Do not leave TODO: Marks or comments in code. The code is more readable if there are no unnecessary comments in the code.
- Describe clearly in the pullrequest what the code does and which problem is solved. New features can be submitted as issues.

# Code of Conduct

Everyone may feel invited to participate in this programming language.

However, there are a few small rules to follow:
- New features should be linguistically JAVA adhered to.
- New code must always be reviewed by a code review and the pull request should explain exactly why the code should be changed.
- Flavor questions are not to be found in the review. Neither from the reviewer nor from the requestor. This only consumes resources and quickly gives the feeling of a request to a perceived knowledge transfer.
- In the JAVA and C scene there is too much narcissism. There is no narcissism here! Quickly more code or more complex code is written than necessary. This is normal and is part of the development process. Be kind!

#tldr:
- Short CoC: OSIC developer are really nice guys and women!
- Shorter CoC: OSIC developer are the one who are what her dogs thinks they are!!
