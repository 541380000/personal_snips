#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <fcntl.h>
#include <csignal>
#include <utility>
#include <unistd.h>
#include <chrono>

using namespace std;

// The client send a request to apply for a bunch of seq number
// The server respond with the seq number range assigned to the client
// Code is from Linux-UNIX系统编程手册

#define SERVER_FIFO_NAME "/tmp/FILE_SERVER_NAME_20220309"
#define CLIENT_FIFO_NAME_TEMPLATE "/tmp/CLIENT_PID_%020d"
#define CLIENT_FIFO_NAME_MAX_LEN (sizeof(CLIENT_FIFO_NAME_TEMPLATE) + 20)

class FIFO final{
public:
    explicit FIFO(string name = SERVER_FIFO_NAME): name(std::move(name)), created(false), fifoFd(0), dummyFd(0) {};
    bool create(mode_t mode = S_IRUSR | S_IWUSR | S_IWGRP){
        umask(0);
        cout << "creating -> " << name << endl;
        // if exists, just go head
        if(mkfifo(name.c_str(), mode) == -1 && errno != EEXIST){
            perror("mkfifo");
            return false;
        }
        created = true;
        return true;
    }

    bool openToReadOrWriteBlock(int flag){
        if (!created){
            perror("haven't create fifo");
            return false;
        }
        this->fifoFd = open(name.c_str(), flag);
        if (fifoFd == -1) return false;
        // open it again with write flag, so that if all clients close the fd, we do not meet EOF, and can continue to wait for read on the FIFO
        openDummyFd();
        // ignore SIGPIPE, so that when writing to a FIFO without a reader, an error is returned to write, instead of occurrence of a signal
        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR){
            perror("signal");
            return false;
        }
        return true;
    }

    void openDummyFd(){
        dummyFd = open(SERVER_FIFO_NAME, O_WRONLY);
    }

    [[nodiscard]] int getFd() const{
        return fifoFd;
    }

private:
    string name;
    bool created;
    int fifoFd;
    int dummyFd;
public:

    virtual ~FIFO() noexcept = default;

    FIFO(const FIFO&) noexcept = delete;
    FIFO& operator= (const FIFO&) = delete;

    FIFO(FIFO&&) = delete;
    FIFO& operator= (FIFO&&) = delete;
};

struct Request{
    int32_t pid;
    uint32_t seqLen;        // how many seq number the process want
};

struct Response{
    uint64_t seq;           // the start of seq number assigned to the process
    uint32_t seqLen;        // len of requested seqLen
};

char clientFIFO[CLIENT_FIFO_NAME_MAX_LEN];

int main(){
    auto t1 = chrono::high_resolution_clock::now().time_since_epoch().count();
    for (int i=0; i<=10; ++i)
        if (fork() == 0){
            FIFO fifo;
            fifo.create();
            fifo.openToReadOrWriteBlock(O_WRONLY);
            int fd = fifo.getFd();
            Request request{getpid(), 10};
            write(fd, &request, sizeof(request));
            // write to fd
            snprintf(clientFIFO, CLIENT_FIFO_NAME_MAX_LEN, CLIENT_FIFO_NAME_TEMPLATE, request.pid);
            cout << clientFIFO << endl;
            FIFO readFIFO(clientFIFO);
            readFIFO.create();
            readFIFO.openToReadOrWriteBlock(O_RDONLY);
            int readFd = readFIFO.getFd();
            Response response{};
            if (read(readFd, &response, sizeof(response)) != sizeof(response)){
                perror("read");
            }
            printf("Get Seq Number from %ld to %ld\n", response.seq, response.seq + response.seqLen);
            close(readFd);
            unlink(clientFIFO);
            return 0;
        }

    FIFO fifo;
    fifo.create();
    fifo.openToReadOrWriteBlock(O_RDONLY);
    int fd = fifo.getFd();
    Request request{};
    Response response{};
    uint64_t nextSeq = 0;

    while(true){
        cout << "reading" << endl;
        if (read(fd, &request, sizeof (request)) != sizeof(request)){
            printf("Error Reading Request, discard\n");
            continue;
        }
        snprintf(clientFIFO, CLIENT_FIFO_NAME_MAX_LEN, CLIENT_FIFO_NAME_TEMPLATE, request.pid);
        cout << clientFIFO << endl;
        auto clientFd = open(clientFIFO, O_WRONLY);
        if (clientFd == -1) {
            perror("clientFd open");
            continue;
        }
        response = {nextSeq, request.seqLen};
        if (write(clientFd, &response, sizeof(response)) != sizeof(response)){
            perror("write");
            continue;
        }
        nextSeq += request.seqLen;
        if (close(clientFd) != -1){
            cout << "Client closed fifo. Cannot close fifo fd, continue..." << endl;
        }
        if (nextSeq == 100) break;
    }
    auto t2 = chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "using insert cost: " << t2 - t1 << endl;
}
