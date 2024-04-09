import json
import serial

ser = serial.Serial('COM12', 9600, timeout=1)
user_t = {
    "username": "X",
    "password": "x",
    "consumed": 0,
}

users = [{"username":"X","password":"x","consumed":0}]


def handle_get_user(username):
    for user in users:
        if user['username'] == username:
            response = {
                'username': username,
                'password': user['password']
            }
            print(json.dumps(response).encode())
            ser.write((json.dumps(response)+"\n").encode())
            break

def handle_set_consumed(username, consumed):
    for user in users:
        if user['username'] == username:
            user['consumed'] = consumed
            ser.write(('ack'+"\n").encode())
            break

def handle_add_user(username, password):
    users.append({
        'username': username,
        'password': password,
        'consumed': 0
    })
    ser.write(('ack'+"\n").encode())

while True:
    line = ser.readline().decode().strip()
    if line:
        print(line)
        data = json.loads(line)
        print(data)
        command = data['command']
        if command == 'getUser':
            print("getuser")
            print(data["username"])
            handle_get_user(data['username'])
        elif command == 'setConsumed':
            print("set consumed")
            print(data["username"])
            handle_set_consumed(data['username'], data['consumed'])
        elif command == 'addUser':
            print("adduser")
            handle_add_user(data['username'], data['password'])