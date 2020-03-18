from flask import Flask
from flask import request, render_template
from os import system as sys
import json
app = Flask(__name__)
app.debug = True
users = {}

# helper functions
def read():
    try:
        with open('/dev/hello', 'r') as f:
            data = f.read()
        return (True if data[0] == 'y' else False, data[1:])
    except:
        return (False, '')

def write(s):
    with open('/dev/hello', 'w') as f:
        f.write(f':{s}')

def write_key(key):
    with open('/dev/hello', 'w') as f:
        f.write(f'~{key}')

# user section ------------ 
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/register_form')
def register_form():
    return render_template('register_form.html')

@app.route('/register', methods=['POST'])
def register():
    with open('users.json', 'r') as f:
        users = json.load(f)
    user = request.form['user']
    password = request.form['pass']
    if user in users.keys():
        return render_template('message.html', msg = "Username already taken")
    else:
        users[user] = password
        with open('users.json', 'w') as f:
            json.dump(users, f)
        return render_template('message.html', msg = "Registration successful")


@app.route('/login_form')
def login_form():
    return render_template('login_form.html')
 
@app.route('/login', methods=['POST'])
def login():
    user = request.form['user']
    password = request.form['pass']
    with open('users.json', 'r') as f:
        users = json.load(f)
    if user not in users or users[user] != password:
        return render_template('message.html', msg = 'Login credentials wrong')
    else:
        return render_template('user_portal.html', user = user)

@app.route('/data', methods=['POST'])
def data():
    user = request.form['user']
    key = request.form['key']
    write_key(int(key))
    valid, data = read()
    if not valid:
        return render_template('invalid.html')
    else:
        user_data = json.loads(data)
        if user in user_data.keys():
            return render_template('edit_data.html',user = user, data = user_data[user], key = key)
        else:
            return render_template('edit_data.html',user = user, data = '', key = key)

@app.route('/edit', methods=['POST'])
def edit():
    user = request.form['user']
    user_data = request.form['data']
    key = request.form['key']
    write_key(int(key))
    valid, data = read()
    data = json.loads(data)
    data[user] = user_data
    write(json.dumps(data))
    return render_template('message.html', msg = 'Successfully updated')
    

# admin section
@app.route('/admin')
def admin():
    return render_template('admin.html')

@app.route('/admin/key', methods=['POST'])
def admin_verify():
    key = request.form['key']
    password = request.form['pass']
    print('key is ', key)
    # try:
    # with open('/dev/hello', 'w') as f:
    #     f.write(f'~{int(key)}')
    write_key(int(key))
    valid, data = read()

    if valid and password == "adminpass":
        return render_template('valid.html', data = data)
    else:
        return render_template('invalid.html')
    # except:
    #     return render_template('invalid.html')

@app.route('/reset')
def reset():
    sys('rmmod chardev; insmod chardev.ko; cat /dev/hello > key.txt;')
    with open('key.txt', 'r') as f:
        key = f.read()
    sys('rm -rf key.txt')
    write('{}')
    return render_template('key.html', key = key)

if __name__ == "__main__":
    app.run()