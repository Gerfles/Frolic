


(dap-register-debug-template
  "CPPDBG Frolic"
  (list :type "cppdbg"
        :request "launch"
        :name "cpptools::Run Configuration"
        :MIMode "gdb"
        :program "${workspaceFolder}/build/frolic"
        :cwd "${workspaceFolder}"))


(dap-register-debug-template
  "LLDB::Run frolic"
  (list :type "lldb-vscode"
        :cwd "${workspaceFolder}"
        :request "launch"
	:program "${workspaceFolder}/build/frolic"
        :name "LLDB::Run"))



(dap-register-debug-template
  "LLDB::Run vscode frolic"
  (list :type "lldb-vscode"
	:cwd "${workspaceFolder}/build"
	:args nil
        :request "launch"
	:name "Debug"
	:program "${workspaceFolder}/build/frolic"))


 ;;; default debug template for (c++)
  (dap-register-debug-template
   "C++ LLDB frolic"
   (list :type "lldb-vscode"
         :cwd nil
         :args nil
         :request "launch"
         :program nil))




{
  "type": "lldb-vscode",
  "request": "launch",
  "name": "Debug",
  "program": "/tmp/a.out",
  "args": [ "one", "two", "three" ],
  "env": [ "FOO=1", "BAR" ],
}
