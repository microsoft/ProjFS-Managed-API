# SimpleProviderManaged

This project is a simple ProjFS provider written in C#.  It projects the contents of one directory (the "source")
into another (the "virtualization root").

## Command line parameters
`SimpleProviderManaged.exe --SourceRoot=<path to source> --VirtRoot=<path to virtualization root>`

Where:
* `--SourceRoot` specifies the path to a directory whose contents you wish to project.
* `--VirtRoot` specifies the path to a directory into which the contents of `SourceRoot` are projected.  When the
provider is running, the contents of `SourceRoot` will appear to exist in `VirtRoot`.
  * Note that `VirtRoot` cannot be a descendant or ancestor of another virtualization root.  If another instance of
  SimpleProviderManaged (or some other ProjFS provider) is running with a virtualization root that is an ancestor
  or descendant of `VirtRoot`, then SimpleProviderManaged will fail to start on a `Win32Exception` with the error value
  ERROR_FILE_SYSTEM_VIRTUALIZATION_INVALID_OPERATION.

## Shutting down the provider
To stop the provider hit the `<Enter>` key in the provider's console window.

## Notices
This project calls [Serilog](https://serilog.net/) APIs, which are under the
[license](https://github.com/serilog/serilog/blob/dev/LICENSE) for Serilog.