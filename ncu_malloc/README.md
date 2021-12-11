# ncu_malloc

不知道 `writeup` 要寫些什麼比較好，乾脆來寫個玩具 `malloc`。
雖然參考了不少前人的實作，但是自己寫一次真的可以領悟到 `malloc` 的概念，之後會寫一篇簡單的心得紀錄這個小玩具的實作細節。

心得文目前還在想怎麼寫XD，不過可以先驗證成果

```bash
$ gcc -o ncu_malloc.so -fPIC -shared ncu_malloc.c
$ export LD_PRELOAD=$PWD/ncu_malloc.so
```
利用 `LD_PRELOAD` 來載入我們的 `.so` 檔案，並且執行一些常見指令來測試 `malloc` 是否有正常運作。

- [LD_PRELOAD 的簡單用法](https://www.52coder.net/post/ld-preload)

以最常用的 `ls` 為例

```bash
$ ls
main  main.c  ncu_malloc  README.md  src  syscall_add.sh  test.c
```

如果沒有報錯就是成功了。
如果想要改回原本系統預設的 `malloc` 則是使用 `unset`

```bash
$ unset LD_PRELOAD
```

即可恢復正常

### reference
- [如何實現一個 malloc](https://blog.codinglabs.org/articles/a-malloc-tutorial.html#21-linux%E5%86%85%E5%AD%98%E7%AE%A1%E7%90%86)
- [trace 30個基本Linux系統呼叫第二十一日：brk](https://ithelp.ithome.com.tw/articles/10186995)
- [memalloc](https://github.com/arjun024)
