import socket
import time
import threading

def send_echo_request(host, port, message, i):
    # print(str(i) + 'th thread')
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        s.sendall(message.encode('utf-8'))
        response = s.recv(1024)  # Adjust buffer size if needed
        # print(response.decode());
        return response

def load_test(host, port, message, num_requests, concurrency):
    threads = []
    start_time = time.time()

    for i in range(num_requests):
        while len(threads) >= concurrency:
            # Wait for any thread to finish if we reach the concurrency limit
            for thread in threads:
                if not thread.is_alive():
                    threads.remove(thread)

        thread = threading.Thread(target=send_echo_request, args=(host, port, message, i))
        threads.append(thread)
        thread.start()

    # for thread in threads:
    #     if not thread.is_alive():
    #         threads.remove(thread)
    # Wait for all threads to finish
    # for thread in threads:
    #     thread.join()
        
    for i in range(len(threads)):
      threads[i].join()
      # print(str(i) + 'th thread finish')

    end_time = time.time()
    total_time = end_time - start_time
    requests_per_second = num_requests / total_time
    print(f'Total requests: {num_requests}, Total time: {total_time:.2f} seconds, '
          f'Requests per second: {requests_per_second:.2f}')

if __name__ == "__main__":
    HOST = 'localhost'
    PORT = 9001
    MESSAGE = "Hello, echo server!"
    NUM_REQUESTS = 10000
    CONCURRENCY = 10000  # Number of concurrent threads

    load_test(HOST, PORT, MESSAGE, NUM_REQUESTS, CONCURRENCY)
