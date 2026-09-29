from websockets.sync.server import serve


def echo(connection):
    message = connection.recv()
    connection.send(message)


if __name__ == "__main__":
    with serve(echo, "127.0.0.1", 19791, compression=None) as server:
        print("READY", flush=True)
        server.serve_forever()
