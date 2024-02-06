'''
Dillon Jackson
dzjxb2
10/8/23

This is the server side of ChatRoom Version 1. It utilizes the socket
api to processes client requests and sends messages back. It keeps
track of user accounts and who is logged in.

The program implements 4 commands: login, newuser, send and logout

Account information is presisted with the required users.txt file

An attempt was made to make the various aspects of the project loosely coupled to make V2 easier.

'''


import os.path
import socket


class Server:
    accounts = {}
    online = []

    def __init__(self, address, file):
        self.account_file_name = file
        self.address_tuple = address

        if os.path.exists(file):
            self.file_to_map()
        else:
            self.accounts = {}

        self.socket = socket.socket()  # construct socket object
        self.socket.bind(self.address_tuple)  # bind to address and sock num tuple

    def file_to_map(self):
        with open(self.account_file_name, mode="rb") as open_file:
            for b_line in open_file:
                line = b_line.decode("utf-8")
                user, password = str(line).strip("()\n\r").split(sep=",")
                user = user.strip()
                password = password.strip()
                self.accounts[f"{user}"] = self.Account(user, password)

    def map_to_file(self):
        account_list: list = [f"({value.username}, {value.password})\r\n".encode("utf-8")
                                      for key, value in self.accounts.items()]
        with open(self.account_file_name, "wb") as open_file:
            open_file.writelines(account_list)

    # The main server loop
    def run(self):
        while True:
            self.socket.listen(1)
            session_socket, return_address = self.socket.accept()
            session = self.Session(session_socket, return_address)
            while session.socket is not None:
                message = session.socket.recv(1024).decode('utf-8')
                if len(message) < 1:  # A empty message is sent if client terminates unkindly
                    print("Possible client error detected, dropping connection")
                    break
                session.request = self.Request(message) # build request
                try:
                    func = self.get_function(session.request)  # get request function
                except Exception as e:
                    print(f"Function error {e}, dropping request")
                    session.f_queue = []
                    del func
                    del message
                    continue
                func(session)
                while len(session.f_queue) != 0:  # process the function queue for a session
                    f = session.f_queue.pop(0)
                    f(session)
                del func
                del message
            del session

    # Data class to hold request information
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

    # Data class to keep track of basic account information
    class Account:
        def __init__(self, username, password):
            self.username: str = username
            self.password: str = password

        def __str__(self):
            return f"Username: {self.username} Password: {self.password}"

        def __repr__(self):
            return f"Username: {self.username} Password: {self.password}"

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
        print(self.accounts)
        session.request.response = "it printed yo"
        session.f_queue.append(self.single_cast)

    # More behind the scenes admin function
    def print_file(self, session):
        with open(file, mode="rb") as open_file:
            content = open_file.read()
            print(content)
            session.request.response = "it printed yo"
            session.f_queue.append(self.single_cast)

    # Required login function
    def logon(self, session):
        user_name, password = session.request.data.split()
        # check if user exists
        if user_name not in self.accounts:
            session.request.response = f"Denied. User name or password incorrect"
            session.f_queue.append(self.single_cast)
            return False
        # check if user is online
        elif session in self.online:
            session.request.response = f"User is already logged in"
            session.f_queue.append(self.single_cast)
            return False
        # check if password is right
        elif self.accounts[user_name].password != password:
            session.request.response = f"Denied. User name or password incorrect"
            session.f_queue.append(self.single_cast)
            return False
        # update status to online
        else:
            session.user = self.accounts[user_name]
            self.online.append(session)
            session.request.response = f"login confirmed"
            print(f"{session.user.username} login.")
            session.f_queue.append(self.single_cast)
            return True

    # Required logoff function
    def logoff(self, session):
        if session.user is None:
            session.request.response = f"User is not logged in"
            session.f_queue.append(self.single_cast)
            return False
        elif session not in self.online:
            session.request.response = f"User is not logged in, with unknown cause, terminating connection"
            session.f_queue.append(self.single_cast)
            session.f_queue.append(self.close_session)
            return False
        else:
            session.request.response = f"{session.user.username} left"
            print(f"{session.user.username} logout")
            session.f_queue.append(self.single_cast)
            session.f_queue.append(self.close_session)
            self.online.remove(session)
            return True

    # Required newuser function
    def newuser(self, session):
        # write pickle when newUser
        user_name, password = session.request.data.split()
        if user_name in self.accounts:
            session.request.response = f"Denied. User account already exists."
            session.f_queue.append(self.single_cast)
            return False
        elif session.user is not None:
            session.request.response = f"You are logged in, cannot create new account"
            session.f_queue.append(self.single_cast)
            return False
        else:
            account = self.Account(user_name, password)
            self.accounts[user_name] = account
            self.map_to_file()
            session.request.response = f"New user account created. Please login."
            session.f_queue.append(self.single_cast)
            print("New user account created")
            return True

    def close_session(self, session):
        session.socket.close()
        session.socket = None

    # Required send function
    def send(self, session):
        if session not in self.online:
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
        sockets = [session.socket for session in self.online]
        for s in sockets:
            s.send(session.request.response.encode('utf-8'))


if __name__ == "__main__":
    address = ('127.0.0.1', 10327)
    file = "users.txt"
    print("My chat room server. Version One.\n")
    server = Server(address, file)
    server.run()
