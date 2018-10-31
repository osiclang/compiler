
<img src="https://github.com/OSIClang/compiler/blob/master/logo/logo_with_font_small.png">
OSIC Compiler is a compiler for the OSIC language. OSIC stands for Objectiv Symbolic Instruction Code and should be 
completely compilable to a wide spectrum of machines in addition to a programming language that is very easy to learn.

### Build status
[![Build Status](https://travis-ci.org/OSIClang/compiler.svg?branch=master)](https://travis-ci.org/OSIClang/compiler)

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

### Build Modules

For prce module (Perl Compatible Regular Expressions)
```
sudo apt-get install libpcre3-dev
cd modules/oPcre/
make
```

For mysql connection module
```
cd modules/oMysql/
make
```

If you can not make the file -try installing the mysql sources
```
sudo apt-get install libmysqlclient-dev
```
If you using Debian and got an Error while installation of libmysqlclient-dev, try:
```
sudo apt-get install default-libmysqlclient-dev
```

Windows Platform
----------------

Lemon can build on Windows via [Mingw](http://www.mingw.org/wiki/Install_MinGW),
getting source code and use command

```
mingw32-make
```

### Porting

`lib/os.c` and `lib/socket.c` are supporting POSIX and Microsoft Windows environment. Ensure you have a compatible Windows Build environment installed.

### Use OSIC as compiler

initialize and set parameter to your source code

Example from `main.c`

	struct osic *osic;
	osic = osic_create();
	builtin_init(osic);

	lobject_set_item(osic, 
			 osic->l_modules,
			 lstring_create(osic, "os", 2),
			 os_module(osic));
	osic_input_set_file(OSIC, "your_osic_source.osic");
	osic_compile(OSIC);
	osic_machine_reset(OSIC);
	osic_machine_execute(OSIC);

Everything start with `osic_create()`, this function initialize all parts of OSIC, 
and all required objects. After `osic_create` initialize module 
with `builtin_init`, and manual initialize `os_module`, there is the place you add custom module.  
Use `osic_input_set_file` set OSIC source code file or `osic_input_set_buffer` set source code from string instead file.  
Use `osic_compile` compile source code, use `osic_machine_reset` reset machine and finally `osic_machine_execute` execute code. 

This is not the final way. It would changed in the future.

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
