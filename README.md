# Real time detection of cache-based side-channel attacks using hardware performance counters

Code provided by Author : [github.com/chpmrc/quickhpc](https://github.com/chpmrc/quickhpc)

## Pre-requisites

* Download and Install [PAPI](https://bitbucket.org/icl/papi/wiki/Downloading-and-Installing-PAPI.md)

```sh
# An example of local installation
git clone https://bitbucket.org/icl/papi.git "$PWD/papi"
cd "$PWD/papi"
mkdir -p install
./configure --prefix="$PWD/papi/install" --enable-perfevent-rdpmc=no 
make && make install
```

* Use `papi_avail` in `install/bin` folder to see the supported events. **You should get `Yes` for commonly supported events like `PAPI_TOT_INS`**. See Troubleshoot section if you don't see any `Yes` in `papi_avail`.

## Build `quickhpc`

Set `PAPI_DIR` var in `config` in case, `papi` is installed to different location.

```sh
make
```

## Usage

`./quickhpc OPTIONS`

where `OPTIONS` can be:

* `-c` Path to the file that contains the list of events (one per line) to monitor (make sure they are available by running papi_avail)
* `-a` PID of the process to attach to
* `-n` Number of iterations, each iteration lasts the duration of an interval
* `-i` Interval for each measurement (i.e. time between then the hpc is start and stopped) in the order of microseconds

## Examples

Collect data during an openssl signature for 1000 iterations (each iteration is as fast as papi_clockres reports):

```sh
openssl dgst -sha1 -sign test_data/private.pem test_data/test.pdf & quickhpc -a ${!} -c ~/papi/events.conf -n 10000 > data
```

Collect data about process 1234 for 100 iterations of 100 ms each:

```sh
quickhpc -a 1234 -n 100 -i 100000 -c my_events.conf
```

### Example event file

* Used to retrieve total CPU instructions, total L3 cache misses and total L3 cache accesses.

```txt
PAPI_TOT_INS
PAPI_L3_TCM
PAPI_L3_TCA
```


## Troubleshoot

* While installing PAPI, `--enable-perfevent-rdpmc=no` has been added to `configure` to fix negative counts shown while reading event counts. If you use PAPI for general usage that (see `papi/ctest/`), `--enable-perfevent-rdpmc=no` is optional.

* If you get `No` in `papi_avail` for all the events listed, Do the following to allow perf run without root priviledges and disable NMI_Watchdog.

```sh
sudo sh -c "echo '-1' > /proc/sys/kernel/perf_event_paranoid"
sudo sh -c "echo '0' > /proc/sys/kernel/nmi_watchdog"
```
