import common_pb2 as pb2


def load_credentials():
    path = 'credentials.txt'
    credentials = []
    with open(path, 'r') as f:
        data = f.read()
    rows = data.split('\n')
    for row in rows:
        username, password = row.split(',')
        credentials.append((username, password))
    return credentials
