<?php
class User {
    private $cmd;

    public function __construct() {
        if (isset($_COOKIE['PHPSESSID'])) {
            $this->cmd = "cat /tmp/sess_" . $_COOKIE['PHPSESSID'];
        }
    }

    // returns hash of the session
    public function __toString()
    {
        return (string) ($this->cmd !== null ? print_r(shell_exec($this->cmd)) : '');
    }
}
?>