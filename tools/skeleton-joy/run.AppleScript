
# 0, 11.25, 22.5, 33.75, 45, 56.25, 67.5, 78.75 
# 0, 72.5 , 120.5, 168.5, 216.5, 264.5, 312.5, 360.5
tell application "GIMP" to activate

tell application "System Events"
	keystroke "R"
	keystroke "78.7u5"
	keystroke tab
	keystroke tab
	keystroke "360.5"
	keystroke tab
	keystroke tab
	keystroke tab
	keystroke tab
	keystroke tab
	keystroke tab
	keystroke return
	delay 1
	keystroke "u"
end tell

