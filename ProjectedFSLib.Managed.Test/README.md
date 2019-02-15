# ProjectedFSLib.Managed.Test.exe

## Command line parameters

`ProjectedFSLib.Managed.Test.exe [NUnit parameters] --params ProviderExe=<path to SimpleProviderManaged.exe>`

Where:
* `[NUnit parameters]` are the [normal NUnit parameters](https://github.com/nunit/docs/wiki/Console-Command-Line)
* `--params ProviderExe` specifies the path to the `SimpleProviderManaged.exe` sample/test provider from the
[simpleProviderManaged](https://github.com/Microsoft/ProjFS-Managed-API/tree/master/simpleProviderManaged) project.

## Notes
Each test case creates a source directory and a virtualization root and then uses the SimpleProviderManaged
provider to project the contents of the source directory into the virtualization root.

By default the test creates the source and virtualization roots in the test's working directory.  If you are using
[VFSForGit](https://github.com/Microsoft/VFSForGit) to project GitHub enlistments to your local machine and you try
to run the test under the Visual Studio debugger, then it is possible that the working directory will end up under
your enlistment's virtualization root.  Since one ProjFS virtualization root is not allowed to exist as a descendant
of another one, this will cause the tests to fail.  You can use NUnit's `--work` parameter to the test executable
to change where it creates the roots.

For example, to run the tests under your $TEMP directory:
`ProjectedFSLib.Managed.Test.exe --work=$TEMP --ProviderExe=<path to SimpleProviderManaged.exe>`
