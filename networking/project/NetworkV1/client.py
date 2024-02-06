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


def get_function_keyword(message):
    tokens = message.split()
    if len(tokens) == 0:
        return None, "User entered blank command, please enter a correct function"
    func = tokens[0].strip()
    new_message = " ".join(tokens[1:]).strip()
    return func, new_message


def check_password_string(s):
    tokens = s.split()
    if len(tokens) > 2 or len(tokens) < 1:
        return False, f"Token count greater than 2 or less than 1, Account validation requires 2 tokens"
    if len(tokens[0]) > 32 or len(tokens[0]) < 3:
        return False, f"UserID should be between 3 and 32 characters, inclusive"
    if len(tokens[1]) > 8 or len(tokens[1]) < 4:
        return False, f"Password should be between 4 and 8 characters, inclusive"
    return True, ""


def check_message_string(msg):
    if len(msg) < 1 or len(msg) > 256:
        return False, f"Message length should be between 1 and 256 characters inclusive"
    return True, ""

# Command validation
def validate_string(m):
    func, message = get_function_keyword(m)
    if func is None:
        return False, f"{message}"
    if func == "login":
        valid, msg = check_password_string(message)
        if not valid:
            return False, f"Invalid login format: {msg}"
        else:
            return True, ""
    elif func == "newuser":
        valid, msg = check_password_string(message)
        if not valid:
            return False, f"Invalid newuser format: {msg}"
        elif log_status:
            return False, f"You are logged in, cannot create new account"
        else:
            return True, ""
    elif func == "send":
        if not log_status:
            return False, f"Denied. Please login first."
        valid, msg = check_message_string(message)
        if not valid:
            return False, f"Invalid message format: {msg}"
        else:
            return True, ""
    elif func == "logout":
        if not log_status:
            return False, f"Denied. Please login first."
        return True, ""
    elif func == "print_users":
        return True, ""
    elif func == "print_file":
        return True, ""
    else:
        return False, f"{func} is not a valid function, please try again"


if __name__ == '__main__':
    while True:
        print("My chat room client. Version One\n")
        client_socket = socket.socket()
        binding = ('127.0.0.1', 10327)
        client_socket.connect(binding)
        log_status: bool = False
        while True:
            message = input()
            if len(message) > 1024 or len(message) < 1:
                print("Command must be between 1 and 1024 characters inclusive")
                continue
            validation, validation_response = validate_string(message)
            if not validation:
                print(f"{validation_response}")
                continue
            client_socket.send(message.encode('utf-8'))
            data = client_socket.recv(1024).decode('utf-8')
            print(data)
            if data == "login confirmed":
                log_status = True
            keyword = get_function_keyword(message)
            if keyword[0] == "logout":
                break
        client_socket.close()
        break
