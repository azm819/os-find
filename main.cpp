#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <cstring>
#include <queue>
#include <unistd.h>
#include <wait.h>

using namespace std;

int inum = -1;
int nlink = -1;
char comp = ' ';
char *pathToProg = "";
string name = "";

off_t sizeOf;

void setData(int argc, char *argv[]) {
    for (int i = 2; i < argc; ++i) {
        if (argv[i] == "-inum") {
            inum = atol(argv[++i]);
            continue;
        }
        if (argv[i] == "-name") {
            name = argv[++i];
            continue;
        }
        if (argv[i] == "-size") {
            comp = argv[i + 1][0];
            sizeOf = atol(argv[++i] + 1);
            continue;
        }
        if (argv[i] == "-nlinks") {
            nlink = atoi(argv[++i]);
            continue;
        }
        if (argv[i] == "-exec") {
            pathToProg = argv[++i];
            continue;
        }
        cout << "Unexpected flag";
        exit(1);
    }
}

void execute(char *path) {
    pid_t pid;
    switch (pid = fork()) {
        case -1:
            perror("fork");
            break;
        case 0: {
            //child
            char *args[3] = {pathToProg, path, nullptr};
            if (execve(pathToProg, args, environ) == -1) {
                perror("execve");
                exit(1);
            }
            break;
        }
        default:
            //parent
            int result;
            if (waitpid(pid, &result, 0) != -1) {
                cout << "Result: " << result << '\n';
            } else {
                perror("waitpid");
            }
    }
}

void proccess(char *pathName) {
    if (pathToProg != "") execute(pathName);
    printf("%s\n", pathName);
}

bool compare(ino_t _inum, nlink_t _nlink, off_t _size, const string &_name) {
    if (inum != -1 && inum != _inum) return false;
    if (nlink != -1 && nlink != _nlink) return false;
    if (!name.empty() && name != _name) return false;
    switch (comp) {
        case '=':
            return sizeOf == _size;
        case '-':
            return sizeOf > _size;
        case '+':
            return sizeOf < _size;
    }
    return true;
}

void recursiveFinder(char *path) {
    DIR *d;
    struct dirent *entry;
    struct stat st;
    char dir_name[256];
    char wrk[256];
    int rc;
    queue<char *> dirs;
    strcpy(dir_name, path);
    d = opendir(dir_name);
    if (d == NULL) {
        printf("Ошибка при открытии директории %s\n", dir_name);
        return;
    }

    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        strcpy(wrk, dir_name);
        strcat(wrk, "/");
        strcat(wrk, entry->d_name);
        rc = stat(wrk, &st);
        if (rc != 0) {
            printf("Ошибка при вызове stat('%s')\n", wrk);
            continue;
        }
        if (S_ISREG(st.st_mode) && compare(st.st_ino, st.st_nlink, st.st_size, string(entry->d_name))) {
            proccess(wrk);
            continue;
        }
        if (S_ISDIR(st.st_mode) && !(entry->d_name == ".." || entry->d_name == ".")) {
            dirs.push(entry->d_name);
        }
    }

    while (!dirs.empty()) {
        char *dir = dirs.front();
        dirs.pop();
        recursiveFinder(dir);
    }

    closedir(d);
}

int main(int argc, char *argv[]) {
    if (argc % 2) {
        cout << "Wrong number of args";
        exit(1);
    }
    setData(argc, argv);
    recursiveFinder(argv[1]);
    return 0;
};
