'''
Dillon Jackson
dzjxb2
10/8/23

This is the server side of ChatRoom Version 1. It utilizes the socket
api to processes client requests and sends messages back. It keeps
track of user accounts and who is logged in.

The program implements 4 commands: login, newuser, send and logout

Account information is presisted with pickle file saved to the required users.txt file

An attempt was made to make the various aspects of the project loosely coupled to make V2 easier.

A basic try catch was used to catch exceptions that are out of the scope of the project,
but I wanted to address anyways to prevent failure. Such as not using the logout function
in the client and terminating the process abnormally causes the server to fail as well.
I cant account for all errors that COULD happen.
'''


import os.path
import pickle
import socket
import threading
from concurrent.futures import ThreadPoolExecutor


class Request:
    function: str
    response: str
    data: str

    def __init__(self, message):
        tokens = message.split()
        self.function = tokens[0].strip()
        self.data = " ".join(tokens[1:]).strip()


# Data class to keep track of client information
class Session:
    f_queue = []

    def __init__(self, socket, address):
        self.request = None
        self.user = None
        self.socket: socket = socket
        self.address: str = address

'''
A session can: Send a message to itself, send a message to a person, broadcast a message
read a list of people currently online

A server must: update account list
log people on

'''


# Data class to keep track of basic account information
class Account:
    def __init__(self, username, password):
        self.username: str = username
        self.password: str = password

    def __str__(self):
        return f"Username: {self.username} Password: {self.password}"

    def __repr__(self):
        return f"Username: {self.username} Password: {self.password}"


class Server:
    MAX_CLIENTS = 3
    accounts = {}
    online = []

    def __init__(self, address, file):
        self.account_file_name = file
        self.address_tuple = address

        if os.path.exists(file):
            with open(self.account_file_name, mode="rb") as open_file:
                self.accounts = pickle.load(open_file)
        else:
            self.accounts = {}

        self.socket = socket.socket()  # construct socket object
        self.socket.bind(self.address_tuple)  # bind to address and sock num tuple

        self.account_lock = threading.Lock()
        self.online_lock = threading.Lock()

    def get_account_list(self):
        with self.account_lock:
            return self.accounts

    def get_online_list(self):
        with self.online_lock:
            return self.online

    # The server must init data structures, with getters and setters that are mutex locked
    # The server must dynamically open and close threads
    def run(self):
        while True:
            self.socket.listen(self.MAX_CLIENTS)
            with ThreadPoolExecutor(max_workers=self.MAX_CLIENTS) as executor:
                while True:
                    session_socket, return_address = self.socket.accept()
                    executor.submit(self.handle_session, session_socket, return_address)

    def handle_session(self, session_socket, return_address):
        session = Session(session_socket, return_address)
        while session.socket is not None:
            message = session.socket.recv(1024).decode('utf-8')
            session.request = Request(message)  # build request
            func = server.get_function(session.request)  # get request function
            func(session)
            while len(session.f_queue) != 0:  # process the function queue for a session
                f = session.f_queue.pop(0)
                f(session)
            del func
            del message
        del session

    # returns object function to process user request
    def get_function(self, request):
        f = request.function
        functions = {"login": self.logon, "logout": self.logoff, "newuser": self.newuser, "send": self.send,
                     "print_users": self.print_users, "print_file": self.print_file}
        if f not in functions:
            raise Exception(f"{f} is not a valid function")
        return functions[f]

    # Behind the scenes admin function
    def print_users(self, session):
        a = self.get_account_list()
        print(a)
        session.request.response = "it printed yo"
        session.f_queue.append(self.single_cast)

    # More behind the scenes admin function
    def print_file(self, session):
        with open(file, mode="rb") as open_file:
            acc_list = pickle.load(open_file)
            print(acc_list)
            session.request.response = "it printed yo"
            session.f_queue.append(self.single_cast)

    # Required login function
    def logon(self, session):
        user_name, password = session.request.data.split()
        a = self.get_account_list()
        o = self.get_online_list()
        # check if user exists
        if user_name not in a:
            session.request.response = f"Denied. User name or password incorrect"
            session.f_queue.append(self.single_cast)
            return False
        # check if user is online
        elif session in o:
            session.request.response = f"User is already logged in"
            session.f_queue.append(self.single_cast)
            return False
        # check if password is right
        elif a[user_name].password != password:
            session.request.response = f"Denied. User name or password incorrect"
            session.f_queue.append(self.single_cast)
            return False
        # update status to online
        else:
            session.user = a[user_name]
            o.append(session)
            session.request.response = f"login confirmed"
            print(f"{session.user.username} login.")
            session.f_queue.append(self.single_cast)
            return True

    # Required logoff function
    def logoff(self, session):
        o = self.get_online_list()
        if session.user is None:
            session.request.response = f"User is not logged in"
            session.f_queue.append(self.single_cast)
            return False
        elif session not in o:
            session.request.response = f"User is not logged in, with unknown cause, terminating connection"
            session.f_queue.append(self.single_cast)
            session.f_queue.append(self.close_session)
            return False
        else:
            session.request.response = f"{session.user.username} left"
            print(f"{session.user.username} logout")
            session.f_queue.append(self.single_cast)
            session.f_queue.append(self.close_session)
            o.remove(session)
            return True

    # Required newuser function
    def newuser(self, session):
        # write pickle when newUser
        user_name, password = session.request.data.split()
        a = self.get_account_list()
        if user_name in a:
            session.request.response = f"Denied. User account already exists."
            session.f_queue.append(self.single_cast)
            return False
        elif session.user is not None:
            session.request.response = f"You are logged in, cannot create new account"
            session.f_queue.append(self.single_cast)
            return False
        else:
            account = Account(user_name, password)
            a[user_name] = account
            with open(self.account_file_name, "wb") as open_file:
                pickle.dump(self.accounts, open_file)  # append would be better
            session.request.response = f"New user account created. Please login."
            session.f_queue.append(self.single_cast)
            print("New user account created")
            return True

    def close_session(self, session):
        session.socket.close()
        session.socket = None

    # Required send function
    def send(self, session):
        o = self.get_online_list()
        if session not in o:
            session.request.response = f"Denied. Please login first"
            session.f_queue.append(self.single_cast)
            return False
        session.request.response = f"{session.user.username}: {session.request.data}"
        print(session.request.response)
        session.f_queue.append(self.broadcast)
        return True

    # Single user response
    def single_cast(self, session):
        session.socket.send(session.request.response.encode('utf-8'))

    # Server wide responds
    def broadcast(self, session):
        sockets = [session.socket for session in self.get_online_list()]
        for s in sockets:
            s.send(session.request.response.encode('utf-8'))


if __name__ == "__main__":
    address = ('127.0.0.1', 10327)
    file = "users.txt"
    print("My chat room server. Version One.\n")
    server = Server(address, file)
    server.run()
