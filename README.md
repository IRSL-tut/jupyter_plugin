# jupyter_plugin

require XEUS, IPython, jedi, jupyter lab

## Compiling jupyter_plugin

Install XEUS, Install jupyter before compiling

```
cd choreonoid/ext; 
git clone https://github.com/IRSL-tut/jupyter_plugin.git
```
After cloning repository, cmake and make choreonoid in the usual way

Or using ADDITIONAL_EXT_DIRECTORIES

https://github.com/choreonoid/choreonoid/blob/master/CMakeLists.txt#L898

```
cmake <choreonoid_dir> -DADDITIONAL_EXT_DIRECTORIES=<jupyter_plugin_dir>
```

## Install XEUS

XEUS https://github.com/jupyter-xeus/xeus

```bash
export OUTPUT_DIR=/opt/xeus

apt install -q -qq -y wget libssl-dev openssl cmake g++ pkg-config git uuid-dev libsodium-dev

## json
(mkdir json && wget https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz --quiet -O - | tar zxf - --strip-components 1 -C json)
(cd json; cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR} . ; make install -j$(nproc) )
## xtl
(mkdir xtl && wget https://github.com/xtensor-stack/xtl/archive/refs/tags/0.7.5.tar.gz --quiet -O - | tar zxf - --strip-components 1 -C xtl)
(cd xtl; cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR} . ; make install -j$(nproc) )
## xeus / 3.0.5
(mkdir xeus && wget https://github.com/jupyter-xeus/xeus/archive/refs/tags/3.1.1.tar.gz --quiet -O - | tar zxf - --strip-components 1 -C xeus)
(cd xeus; cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR} . ; make install -j$(nproc) )

### xeus-zmq
## libzmq
(mkdir libzmq && wget https://github.com/zeromq/libzmq/archive/refs/tags/v4.3.4.tar.gz --quiet -O - | tar zxf - --strip-components 1 -C libzmq)
(cd libzmq; mkdir build; cd build; cmake -DWITH_PERF_TOOL=OFF -DZMQ_BUILD_TESTS=OFF -DENABLE_CPACK=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR} ..; make install -j$(nproc) )
## cppzmq
(mkdir cppzmq && wget https://github.com/zeromq/cppzmq/archive/refs/tags/v4.8.1.tar.gz --quiet -O - | tar zxf - --strip-components 1 -C cppzmq)
(cd cppzmq; cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR} -DCPPZMQ_BUILD_TESTS=OFF . ; make install -j$(nproc) )
## xeus-zmq
(mkdir -p xeus-zmq/build && wget https://github.com/jupyter-xeus/xeus-zmq/archive/refs/tags/1.1.0.tar.gz --quiet -O - | tar zxf - --strip-components 1 -C xeus-zmq)
(cd xeus-zmq/build; cmake -DCMAKE_PREFIX_PATH=${OUTPUT_DIR} -DCMAKE_INSTALL_PREFIX=${OUTPUT_DIR} -DCMAKE_BUILD_TYPE=Release ..; make install -j$(nproc) )

export PATH=$PATH:/opt/xeus/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/xeus/lib
```

## Install jupyter by deb

### IPython

apt-get install -y ipython3

### jedi

apt-get install -y python3-jedi

### jupyter lab

python3 -m pip install jupyterlab

python3 -m pip install jupyter-console


## Install jupyter by pip

```bash
apt-get install -y python3-pip
python3 -m pip install --upgrade pip
python3 -m pip install ipython
python3 -m pip install jedi
python3 -m pip install jupyterlab
python3 -m pip install jupyter-console
```

Installing to /usr/local
```bash
sudo python3 -m pip install -r requirements.txt
```

## RUN through jupyer

```bash
export JUPYTER_PATH={path/to/install/choreonoid}/share/choreonoid-2.0/jupyter
```

### Default (browser will be launched)

```
jupyter lab
```

### Access without Token

```
jupyter lab --no-browser --ip=0.0.0.0 --port=8888 --NotebookApp.token=''
```

Access http://localhost:8888 by a browser

### Console

```
jupyter console --kernel=Choreonoid
```
