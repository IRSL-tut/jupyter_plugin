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

See https://github.com/IRSL-tut/irsl_docker_xeus/blob/main/local_build5.sh


## Install jupyter by pip

```bash
apt-get install -y python3-pip
python3 -m pip install --upgrade pip
python3 -m pip install --break-system-packages -r requirements.txt
```

Installing to /usr/local
```bash
sudo python3 -m pip install --break-system-packages -r requirements.txt
```

## Install jupyter by deb(not recommended)

### IPython

apt-get install -y ipython3

### jedi

apt-get install -y python3-jedi

### jupyter lab

python3 -m pip install jupyterlab

python3 -m pip install jupyter-console


## RUN through jupyer

```bash
export JUPYTER_PATH={path/to/install/choreonoid}/share/choreonoid-{cnoid-version}/jupyter
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
