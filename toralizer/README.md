Add the `toralize` bash script in you PATH and give it executable permissions

Run

```sh
make
```

Make sure `tor` is running

```sh
sudo systemctl status tor
```

Make a `curl` request without toralize and check you public IP

```sh
curl wtfismyip.com
```

This will return your public IP

Now run `curl` command but add `toralize` before it

```sh
toralize curl wtfismyip.com
```

This will return a different IP than your public IP and proves IP has been masked