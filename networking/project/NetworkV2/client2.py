'''
Dillon Jackson
dzjxb2
10/8/23

This is the client side of ChatRoom Version 1. The main function of the client
is to validate the structure of function calls, and provide an interface for the
client.

To account for unforseen errors outside of the scope of the project a generic try catch was added to prevent
client termination.
'''

import socket
import time
import threading


class Client:

    def __init__(self, binding):
        self.client_socket = socket.socket()
        self.client_socket.connect(binding)
        self.log_status: bool = False
        self.end = False

    def run(self):
        r = threading.Thread(target=self.client_receive())
        r.start()
        while not self.end:
            message = input()
            if len(message) > 1024:
                print("Please limit command to less than 1024 characters")
                continue
            validation, validation_response = self.validate_string(message)
            if not validation:
                print(f"{validation_response}")
                continue
            self.client_socket.send(message.encode('utf-8'))
        self.client_socket.close()
    def get_function_keyword(self, message):
        tokens = message.split()
        func = tokens[0].strip()
        new_message = " ".join(tokens[1:]).strip()
        return func, new_message

    def check_password_string(self, s):
        tokens = s.split()
        if len(tokens) > 2 or len(tokens) < 1:
            return False, f"Token count greater than 2 or less than 1, Account validation requires 2 tokens"
        if len(tokens[0]) > 32 or len(tokens[0]) < 3:
            return False, f"UserID should be between 3 and 32 characters, inclusive"
        if len(tokens[1]) > 8 or len(tokens[1]) < 4:
            return False, f"Password should be between 4 and 8 characters, inclusive"
        return True, ""

    def check_message_string(self, msg):
        if len(msg) < 1 or len(msg) > 256:
            return False, f"Message length should be between 1 and 256 characters inclusive"
        return True, ""

    # Command validation
    def validate_string(self, m):
        func, message = self.get_function_keyword(m)
        if func == "login":
            valid, msg = self.check_password_string(message)
            if not valid:
                return False, f"Invalid login format: {msg}"
            else:
                return True, ""
        elif func == "newuser":
            valid, msg = self.check_password_string(message)
            if not valid:
                return False, f"Invalid newuser format: {msg}"
            else:
                return True, ""
        elif func == "send":
            if not self.log_status:
                return False, f"Denied. Please login first."
            valid, msg = self.check_message_string(message)
            if not valid:
                return False, f"Invalid message format: {msg}"
            else:
                return True, ""
        elif func == "logout":
            if not self.log_status:
                return False, f"Denied. Please login first."
            return True, ""
        elif func == "print_users":
            return True, ""
        elif func == "print_file":
            return True, ""
        else:
            return False, f"{func} is not a valid function, please try again"

    def client_receive(self):
        while True:
            time.sleep(1)
            data = self.client_socket.recv(1024).decode('utf-8')
            print(data)
            if data == "login confirmed":
                self.log_status = True
            if data == "logout":
                self.end = True
                break

# Client is blocking input
# Client needs to know if it is not logged on
if __name__ == '__main__':
    print("My chat room client. Version One\n")
    binding = ('127.0.0.1', 10327)
    client = Client(binding)
    client.run()
