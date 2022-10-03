# jupyter_plugin

require IPython, jedi, jupyter lab


## Install by deb

### IPython

apt-get install -y ipython3

### jedi

apt-get install -y python3-jedi

### jupyter lab

python3 -m pip install jupyterlab

python3 -m pip install jupyter-console


## Install by pip

```
apt-get install -y python3-pip
python3 -m pip install --upgrade pip
python3 -m pip install ipython
python3 -m pip install jedi
python3 -m pip install jupyterlab
python3 -m pip install jupyter-console
```


## RUN through jupyer

```
export JUPYTER_PATH={path/to/install/choreonoid}/share/choreonoid-1.8/jupyter
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

