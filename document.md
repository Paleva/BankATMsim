
# Usage:

* To compile use `make` in the root directory *(Makefile provided in root)*
* First, run the bank application, and then use the client application to connect in a second terminal window

# General information:

* In the "src" directory there are the main .c files
* In the "db" directory there is a .txt file used as a "database"


## Components
### Client

* Connects to the server (ATM) over sockets and uses them for communaction.
* Provides UI

### Server(ATM)

* Handles clients requests and send them to the bank. Acts as a middle man for verification.
* Does most of the heavy lifting regarding the authentication

### Bank

* Manages the bank's data, stores users and deposit/withdrawals in the "database".
* Sends requested information to the server (ATM).

# Communication protocol

* HTTP-based protocol *(Frankensteined)*

### Requests
1. GET /
    * GET /withdraw
    * GET /balance
    * GET /exit
    * GET /login
2. PUT /
    * PUT /deposit
    * PUT /create
3. EXIT
    * EXIT /exit

# Actions

* Firstly, I thought about the basic architecture of the applications and how they should communicate with each other
* Secondly, writing the code and trying to figure out how everything works *(basically experimenting)*
* Thirdly, a lot, and I mean a lot, of debugging and frustration from desynchronization
* Documenting features and the project itself

# Challenges

* General C things like pretty obvious segfaults
* Figuring out how to connect everything *(IPC)*
* Cluttered code as of now *(05-14)*
* Trying to implement the ideas I had and seeing that they do not work
* Trying to understand how things work
* Session handling

# Features that should be added
- [ ] Full input validation
- [ ] Improved "UI"
- [ ] A smarter way of handling multiple sessions
- [ ] Clean up the code into multiple files. For now it's a huge mess