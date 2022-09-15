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

def get_create_market_message(instrument_id, precision, user_names):
    instrument = pb2.Instrument(id=instrument_id, precision=precision)
    create_msg = pb2.CreateMarket(instrument=instrument)
    for user_name in user_names:
        create_msg.user_name.append(user_name)
    message = pb2.Message(type=['CREATE_MARKET'], create_market=create_msg)
    return message
