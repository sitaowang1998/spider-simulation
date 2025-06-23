# Spider Simulation Benchmark
## Setup
1. Start a MariaDB storage on a node using
```
docker run \
        --detach \
        --rm \
        --name spider-storage \
        --env MARIADB_USER=spider \
        --env MARIADB_PASSWORD=password \
        --env MARIADB_DATABASE=spider-storage \
        --env MARIADB_ALLOW_EMPTY_ROOT_PASSWORD=true \
        --publish 3306:3306 mariadb:latest
        --max_connections=1024
```
2. Install dependencies
```
./submodules/spider/tools/scripts/lib_install/linux/install-dev.sh
task deps:lib_install
```
3. Build the experiment executable
```
task build:simulate
```

## Execute experiments
1. Run the command
```
./build/spider-simulate/src/spider-simulation --storage-url "jdbc:mariadb://<mariadb_host>:3306/spider-storage?user=spider&password=password" --num-threads <num_threads> --num-workers <num_workers> --num-jobs <num_jobs>
```
2. Clean up
```
mysql --host <mariadb_host> --port 3306 -u spider -p spider-storage
delete from jobs;
delete from drivers;
```

## Note
### TCP connections
When a TCP connection is closed, the side that initiates the close (usually the client) goes into `TIME_WAIT` to ensure late packets are discarded. Linux holds the socket for ~60 seconds, and while in this state, the connection is technically open, and the sockets and ports cannot be reused.
When running experiments with large number of workers, the simulation creates and opens storage connections frequently, and runs out of TCP connections because of the wait.
To solve this, set the system to reuse TCP connection in `TIME_WAIT`:
```
sudo sysctl -w net.ipv4.tcp_tw_reuse=1
```
Remember to set it back to 0 after the experiment.

