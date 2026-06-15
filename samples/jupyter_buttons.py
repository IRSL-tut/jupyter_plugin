import cnoid.JupyterPlugin

## add button and toggle
cnoid.JupyterPlugin.JupyterBar.instance().addUserButton('hoge')
cnoid.JupyterPlugin.JupyterBar.instance().addUserToggleButton('toge')

## callback(button)
clicked = False
def hoge():
   global clicked
   clicked = True
cnoid.JupyterPlugin.JupyterBar.instance().sigClicked('hoge').connect( hoge )

## callback(toggle)
state = cnoid.JupyterPlugin.JupyterBar.instance().getUserButtonState('toge')
def toge(on):
   global state
   state = on
cnoid.JupyterPlugin.JupyterBar.instance().sigToggled('toge').connect( toge )
