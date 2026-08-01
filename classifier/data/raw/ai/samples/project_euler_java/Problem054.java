// Project Euler 54: count poker hands won by player one in the supplied deal set.
import java.io.ByteArrayInputStream;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.zip.GZIPInputStream;

public class Problem054 {
    // The canonical Project Euler deal file, gzip-compressed to keep this program self-contained.
    private static final String COMPRESSED_DEALS = "H4sICLyxbWoCA3AwNTRfcG9rZXIudHh0AF2dSbKkyhJD57GKXAM9QwwGGIww2P9aPqEjceu/YdXNpInw8EaSe07r77l/5/qb9193/8bt19y/fvu1929ZS7/+"
            + "lq3+c3k/sP7G9dfvv2n7PdvvvEu7/8b9N+jrx/279vr/x/prtt90l+f95P57r3Ddv4fr66/n/X64cKn3i++3jvf670XeD+h23V3qn27dcatfnPQ872PU59nL"
            + "8H5SV34foNnrn96L9Hd9kuW973v3tX7gvcV73/cWnT453vWZR73I8D7/Vv+z0+Nda33Bcy/v470X6XXZRp+Zt3qdRc/8Plt9x73+5/tq77oNLJTWqtECvk/V"
            + "377v+16Dlrddy6V3fJ/8XajmfR6916nnfNfqrCv2m+96r3f9W135fZKufqa0a32j93ne1XjX8P1Yp1fu6tKVSfcaddP3M+/t3me7tGX9+92tLuylJ6nro8eY"
            + "9ILXXd7/edf2XYT37oP24tHi1GXfSqO/TlrAR4/3vvistWq3+lTv0jV6yGOvb9TIMOqL7HUH379OLJGen11+DWbc6g4+Wtu6sLtuvdbrn9VCylLXpH79fdl3"
            + "tRevYb3F+0a9/v9dxla74BXWq01rGbRf1VRkGO92VIOUebzP/P7ne+umWoKuttWnGrUp015OLan22lbh1Vhl7Zss+a5XO/QMi9akmvRW97eXhfda8FELcuiN"
            + "ur00sv9BRjtrLzgm1ZZem9zqXd6rvRc5dr/dqOWtO7h7B/WC9ZXf98II+7uu1ayHmbXynZ6t1ZNPW73vKXu+ZKh1E98X3HWpu57u9zl77Fz3ZRPPupjlxC1o"
            + "BxvZxqyPPdXSSj1E+u6lLy56hUt+4D1H77daff7Qme30xUUmemk1BrmC9yKtjvys41/P2nvfuJpj9QK2+m5bvUFd51OmVQ+RDOxdkPfic/18ufAVOfXvagx6"
            + "tvqB12K175eMsO6RtgwbeN+3Hnm91KNT8P61Ho29rvnrVTrt+yJD5Xg+evFOfqMaqpZ60ca1esFG9x3WekIfGQaPPcrtzHJKnJRDq1qP8FYfoPoZ2XbdIxn2"
            + "rBM01j2tz8xeN+9T6UVO2WGj/eLz8uTlkgu9Nrsd3PgkD/++0anXOfQ6s7ag1VP18XWcuPP2OeIZnup8yqF17nXlU+91yXiGuvLlfTZcX90gmeKlNamu/q77"
            + "e8iMmzsbp/M+VWOr362uSWt76bCccr/vare6Mhs06lAPuAWZx3gXosChU9Dv9oSLX7PgtBs5z4cX1160u/2kX/lWUNBNe+1y9Spaos7mXa/56B3r9bfSybVW"
            + "662BwOeo5S5bXclRZx8/ecoYHoXa4X1fdp+12liEaiQ6NfIbm6PkI7Pn1HT1Aer+tqtNBd81xQO/8Qg7fLQmk8x4lhN+8Fcrj6dV0hqylTVobnV/r9shmEi0"
            + "aLPO+jFFuq0uQiN/hTPvN8ejM29xyO0Q7+TK3osXFn/cbYeD4jLn/dRJ6bVcnc7CpcDEhp6yyYl4QQaipx0cGRW7ZRJ8/trt62r0X2vE6Xcfz1HO/9ReKKmo"
            + "3211zUW+lFcmM3nvO+krk/6nue3TRl382qpNttx6rW/KFoyylncH8ZDn7shOrMFVXrIN4vIoSxsUl7ObRZlSfc0etyyXxTF8Yxlbc+HudEir+emZm63GhYtY"
            + "ttpJdvKKz+616rVrxMpzc+o15YTi6xbd8dDz9DLvYau+vccwdGwf+U85Olnd5qD/6MlHXae/uZGeSkevI0LpRg9eUZZzajXazYnopDjSKw5OOo+8yKC9a7Wq"
            + "fb2RPJI2sV3xQno7faDT/p74dvl/pa85FG8WtPqNLr3vIo96+iRW397pw48Wv73JjnBKhb1rEw3PzYdXmVI9KaeealEUu+QqJ8W+TnbV7jZ+0uxDXzzkn3ut"
            + "KpY23M6RDp3395kPnamDmMXD60Y6XzUu9LpRq4yR0NA4+awZMinEqb+OckGHznvdwTteSFs5knX7lXUWdAoG2RIpdIunXesO1nAm9y47lKeSCdnH6hCdHARd"
            + "v5e19MoZBjIEGW0vy5xXe2DSuUOHurn91qP8Eif00upRa1zykErpa/x9ZO1YcscRSyVST6hW79ydBfXkk1igMkbylk5urSET1m5OymOnBHpCXqd0Sy9VCNyH"
            + "colhdyp1yJG+HmnZ7PkvRZx+c/4/q8Y5N3uDh2RYFjs7j6r7O+1OBgad054yRHndpZNCxoKp91qNdlcc1PNcstVlZfHrplQD1tnHdTRasUsH8NRSv759WH0k"
            + "B1322lyU1dPxXpmM6HY0PHdvxKA4+GihLvn2gVC4JX4pLgw6rVcO17K7YHnXqonvYsEXbdOA/ejKz+1D5AOyOwkc93qOalqOD9RLtTh27e+xO+m6lBicvI7M"
            + "/q0IFg4vaczmvLHf/EazFmHED++OpyfF7FrIanot3aj/bDa7gkYxtN+c1l6bvdO82+fMm3OPTtlRS2RfyXULkbrRIzVa3llrW6sYncGRbHn1cXtcHNWM8bpd"
            + "NB2yupnidyMnqU81rmT4cqcqW5qNdL2eI58dmcSglXzIkfb6zI9W/lq9RDjYunG6Mv552V3WOcdWhfsoPWj0kENSLIq1UxVfry8+ChyHMvNBF3xP97k6+ZxI"
            + "SIAalEU3QgNw0anFXLQqNa2Z25z0uyVtIBbc8t6rfcKh/9HxqZ9UIl0Wna9eG4rTI4HXwpYmUcOeRIt84tDeim93rdfr4pgZ2/2u5JDk5IrvHVf70kVedMYY"
            + "dAAb4ikoyl5P6LMZgph0iJrVlvZ+99md/Cxy4N1KPa4r77UebNhZfYwsmpD6nqNW1rLoNfEPrda2rStT35c83DWUDIzANKguO+VzBnyOzhG+9N2FiVMpx9gD"
            + "y9wGBHpdeU4K3eqlKLRbVSJn8JCHB0tVWC15rxbbbslvKRkI7tUnFEKzd/b2fjWY7l3wCQP1i8xg3IyAdfI5M3aYz/TyQiAJwm2cFz2ro1Uvp/1670PL0qdq"
            + "OHwKlNhsNd+YNgMyo9ZqDJJ2rn6jY7UHnv9JoV+/Qc1FFKbEwKVowYVgbE6fHqyOc1d3p54FoA/C0KMYPe8EykIhfGyuFJYse6/4262kcDatWbnupaxjIHMD"
            + "INJN55hN9QOq6DFLwu5CwqP1vPS+QB9ziogJ7E5xgZpuIfulWJAnrJ/R6QY+ukB4dBJHh/vC9uFgGx5YV64Vrir6IYn3sduoWu3m+92FtEqPNMlLHD6AQqiU"
            + "goIiknIQJure6RxRYPb6FhX6aQ9fSAgPLf6VgmXGUcir9HrO7naAw/x6ZblAT8YA9ZpLdrNTPbhoE6eNnLlePzlkzaBmarqgHETMUX/F/vvAYvNqj/eoTqGE"
            + "aXdjbhyxia28C+s/6tadTeIrkOsuAJSxUz3B7vZfWRlSU0IV0b9VrUHyc+qm1JuNLFzxuj5Vp+0egQU4bi7DCykTWdmSfLgBWpHlTBz51fGxw2zqstRnHohi"
            + "a+znw8wV6Ti5527vfeniSneLvJYrtSXIeaNneKuJg0Sd9GM16vLcYD41M599apKx76473j06tIxsCpmGMitWtX731Cs7tdichrU1stTs60OHus2pFDnYLPTj"
            + "kcc4qeVXP3yrKsaRPUn7KGfb6vUP3Ze4yWp3ertuc27GvTrtwnQ733tWANIC0AeQ0pFO4LUUQ68sbBtnRe32kKvIr16rAXPcyyIPsAhhBmQA3WXfT1Zv80k5"
            + "NuOHH+qryq6A7VMigQi1q63rjaGL9o5aDKs7iNfVkgtPC+6xxOB74IW9YjKNduHanKIcuvsAOq2FalMhDnFKWPuoCDLqrRstOGhYK5x/1rJQpgFS4aPqOrxn"
            + "fzNUcug5rzvhWHGQcqAluV3t8/twQHM4oGN3AuNiVnY1yjeSjcyJlb3e7tEbzalixt3ZV2vCRdwEIPxmYgUETIvjmPLoK3NIKAdoViOpJsewU/otJK1emeT5"
            + "WAP4rOaMelXWZPIwShMRjRL+rrkZJ46XIru79IFL58glEmdcRgVxM5KpamvY4l4B6Aoncui4fbHySSJUz8Jer3yljpjjEgcdlp64kLyrk2OHYFIWIU5EfwIM"
            + "BL+aVxKhAmB1aQ2/5z9lIY9yb0DgZTU/SApRncxdgFip3EkLW4xqjxddnRRdWStwpPeNJn1l3sy+UUY1xkiLkf/VT8WRvExnVGQMdowDQmVEDQWyTbQdbu9y"
            + "v9o5tGItoQZYkEHrP5Bg7AVSoFNYh+VpN0N/p6IzPmqEdtRa4c8P+fZD9kahejn7JSoVc3abeU9In2ljU6q/WjY/zKmLEHCFltf7gkg/cH9b6NHb1dOghwGT"
            + "4az1RvbqSlLagPKRhMNbzUIvASRBp4dgTad4nGX1lj2rOZrhNqHwrvOSpYN2eWDWdNzePcIVtJt5tE5/gsc8lF1jCb0SHqqwSf58lBclLDo10o4QnqbsIAjw"
            + "tHp/Lx20ijPcfmbw2EuuG7M8xeOAsQAHkTtNu89vI4MRzqmnIomVAwQlHoITwoCMt6PwW9Hjxx4yeTK022Dm64H1bH5sTJ2k9xDac8mEOFbz5lDVOdl2bYVv"
            + "J8FugvhNysyhUZqVk2UQQ+xMgXmfdvMdsxzaiWui7s4hunbnV6SRi5CTJXG5ywF0ur5rF3ZYDGVHu5nlWkFsNRsBLpu3EG3/sKUUwliyYlAARsVfjjYZF0+L"
            + "SR+KVkRVDumlGNokiZqFBALZUTVw8BNHCsjzpP8nu+iCszWqjr8w0YZDHyl2tmISFihYu4wBXPJ1h04fadu5O+uDomp0XzJzgIiL56cwl8W2oe1AyfBLOhQF"
            + "BAbqsAtzsbiSKhxJDshp4l4ot6onEAYAbeDBI7n9qf3FC1ERE3k7zEx5O0UTJ5Tt7jbg+uI6erdAoiFWctaEi/L1kfxNZRpXePRGgLREGQAHANuqGwGf1BuB"
            + "twwb5EJ95itl+JFkABS638zRg3uDbkF/jzuUWa0XoMifzfTxRaom336Fbp5JYG6b7iiuDT9DZBlvqw4aH4fCh8nubBJUwXX9jeQjPMBdtGEwa76xOZ1o5CdZ"
            + "yWEF6yjd5wB3U/PiKyFPa8RBdDES63cnaTo4ZYii4Cshp1jvs9nnwFU9PpgmrxtpTo4oChoStrBj5M9nEKQu1QrvuIizbsNjXgZb7MD7rTQ2IS/Fpb0TyV53"
            + "AbqfQEaGQI0j6KmAdQy67AIRgBZIDAI1KZgYPN1CWBQbDkqz6Hlw+E2iwKwqpktKDKvVhmh+hDJ1Cr6IZKYtShjULLK3frU5zavfCFylTbwgvi+4DqxamMwI"
            + "EKe8mpDR213L2imKPzJdhi0rLYg6iKHABQcigdX3JZFobgt4Fi1FIx62oXBeDQd1cdHVejfXZQCM8B0cUqVkxcqcxCxc5aSk4lFMwS3Dzs/EVuKILHYMPXHc"
            + "RttIPh/lDJ8Cp5dHem4rjiZVBJBlwFZX5EwqYyt/NIXQH26j3/AyYF9owMj2R12z2SGYzJe1aI2Iwquhs2lzfQTqCIp4aStV8tdn7lfH0C5mA+gxyatwHIgj"
            + "HawTbJHO4LKbYZnjH3Da5ghuxwJixKlLCTQWLnpbaHSupsWJL+/ptvdW0Trpr5/iaIRbjAysVXa37PYAszAZ2LQGome3+ksihALH3VFH7H7fh4JCqHgbsBSc"
            + "/4RLEkrcWYbnN30SMmQ/pf+IodtSDUoDoYWFYo3AOgX56ah3lLlhLQ9qIlJiA4YFQQ6oMmt15dne+GudwOYHg+DgKJG5metcraxDBqDtcB4754iB/8NhHWIP"
            + "YfOnOxjg5kB/St2BzAnybkiu0gd1uThTt3OVPhXWKZ7dMAscgcwGkBxmql+tMJy0nl1SyiZ1Cme2uc2h43YeVbjzbQIdNO+KjmvSG80h92Fh0Gl0qz0hhgGW"
            + "OOu9zAptRi85dLh0vjuCfN5mH4gOc7QfqMKMdiaxR3uw4K+kR5pvk1OdaTIgNeUMq3NmvLT92D/YJsaGynTYTJpQWYOqPZ8UAdwDwEdIAuUnf1piNqJUCuKK"
            + "aXcmQI0DCtfAw9rvuZaksl7ELTaxq5F09HaBI6FsQcUERduv5uAuSwHLELHfEPAZnLxXLjrvdkF4UbL9KGcKais0nyQ8F8AL7BIpHwolZEt6i0NalyPkLznY"
            + "GH9bM7G9wCZwO9z4E9aGanGiVEw1AVUqJaT4/TwtZA1htFOtYS3WHaB1d7It9VEBBuw3Z9StHh7UovKSt6UvvAUsJOVwK8QV/ZJ1gLomIpBH70s4hrodw3C1"
            + "a6pjwuvtBK9fnfzDWw0B51sUMtunmCqGFPDetzVRrTzPIu5piIyB9IwCSty0q4l+d3ZKmkS984jzQsnQhwRsosQblX3xhE8q3+MjEKWfBLsY5FtQXy9mVbxW"
            + "lJ9TlOSzE11XMWzxvzpkYfgFZdQQ7TGPN2/Oglyw78Y2T5k3RVYn1R+BnsqFWgafabvanVkBQOGBQU6O2xw9khXivkW8um8fCWUTFgkBfMVytz8h8bL9abBb"
            + "ZbkA3USrASQhh7SXPbtCjDiHJwf3BoQ/wi9fmyUEF/75Nvo0yTbmfwLifFtf1yQd7XgAV5TFSs7NOorOvteqPysuwpjgJJGOdHoqCGU0tDDaAy5IFd8cJ3yF"
            + "frJuTTZJInTs5uCkfJaXFlrbBsI17xZw4Lm9zihDWhnkLLMXs1NYfws+WfPbR/iNGtNuB0U+zzaxy6ewEYrET9kOGXooy22jPIfiJ9hhvZMs9glET/F1UdEr"
            + "YwQspYg7d1fW6KkaZUGgrFBOXUBUJagFpwGe00eGSg116Rx1sLe3JcrO5XTKrtWqHolMrI+iSKTGuazVsXQQpZAKgfIECMJ0+39wzkWKBST3Q/TPcIuLciRY"
            + "qiVyX0pR8+NS0B2b6bMxzDUGvNzGVB+Uk6RVqx0jejNERN1tThw+SOyJ6pTkP03yZAtaFNk/hPbZLT55bjjZgqsnlrUhVsjurEInyYycBsG/miwKlCWq+zPq"
            + "zcvXKdOX56ATixZdctxi2lSB0hq5zZRot9p7R+xh5UBjbLZY3Lt9K0D1Cvxe0D2SgE1hbM0jS1HGCh9GaH0i6G1pHPJMhUBeWNylM9iHOkfhZsZEf6X3B7q5"
            + "j34P4HcUMjabDTR93ERit0gfe+wmpMh1rfCR6oBspAke3uh8gckPYtMQIU+54wTXrHqBXL3bzfKgk2/d0VAoZ8bgkAB9J1WAPCHZ44cDnwjeZDkA4LMLBKuF"
            + "Z/g+McuAZleYONItJX4F6AZgLVJeE8HvHh3bPy4UDSesjTzwFb1Bk/aHeXVFeQol7uOiEXQZY5G/erR01CNXsBooztcztKuFCi6FwFVg26XfoNkBtBC/QbAb"
            + "xdLS7EA9eHxtLPU6ZQ4oDR7O8cRLdELzZoJdqF5qogFWWrZN6QGMSenR6KSct4VbRJllt2pikQc2m7aag+tTC6uUK59wa1kdwd3PVRenkP6BPPBGx+7T2glT"
            + "pfeKNhaFe/eRnat704bk9miYyaInxRTajs7gkGgPBKeUPiIWBGOoLjloh2p2mt3G1cG9/+xTyoEG9PK2BB3AR7qOYmv8/ieKFMlICnhyG9CS7qcUjIXSyXXr"
            + "ZkVQp5W59FRj2DQqZZRUqlmsC3L/IC1RSdTpA5oiQrh2s5mOX4plQwqfj/3pneiqC8yYsFE+wqWOVZkcxK2rge7Et1cdRTTnKFKe9CJJB1t4fuzwQV57/ylg"
            + "PzQVRhWulrKUfg2Mv3F/VtoKhE4fMLPpa4M6oc+iUzZypnSaLF/E1KtXIXeiT6QJ42wkUFg9qDJUNcErTX/i6Ol3W11OAsh35ISrBWZPMsbxdvsD3PHy8YZ0"
            + "ZyTqPaq7yZc+icWZzqBpN0JFkjnHJVoBqHphiF732F0oXa5q3TNFNdGkjbQxp2BFN5QlKCiaBJ21AjExxvY4vLObzkofqQOmfkYW2NFPd7tr5lrt5KdP/SsG"
            + "ny0monXJDdRYV6DkyAz5Cml/q1NGtkB9usQ5YBLHbYWV843NfXmDO5icx+KW6ay8ItFcVvcBWR1Ewpaqf5TOHHiHnlbyZPRjkxBm6GMqDpyqWb/NJ5Tj/ynh"
            + "gcFPrcbzoYhutTBi+YjjW9ILCeTbG4KoJxQBYasFJMejyjvhcdIQOlGOrWb0ROm6qwKx3/iPZ5CySOhl1IaclzNANE813M6sxk+xBqDEaqxmPOHxIRceXdmS"
            + "AKic1fDLYvlxsdxRL9jHvVOwdOqq6KPkPCMPoAVjpMsgDU0ARE+QBPoHHYn4wOpVVTgoNgOd3A8DaTGhu0zpTCQ0cJwPL1fheYbVuQpJ++P0u3532CzpGdJN"
            + "djogFmpqY3R7ug8AeOMn0e33+SupFN2FX3J7bdaMPZbmFjSx9Dh0vGzO1CmckOx6vF1+nrfbARZhyAQ462puK+THzZwXa7uksQ70W0l4QU4Pzw4W5/ZY9VvB"
            + "dExRTva38yj5ojKHGUd5C/rqTgqdfdiQJepH7EfAXVnCGSFXc7G5+X1pSzQ9mhWeHbbM/gNNAO8/CaOnMgpQJuQZi1ERZ/UHEM0H2d1O8kdFyT6t32if0qUl"
            + "dEU54RUhXx9G4HRjRaEQWFxMKRre7sZ6VDtPkcqQ4WCWKH5JTY1v7JH97+Y1qB3oWmr+odtE1RWaDq5PjJ1KR8MNCp8fQzBNyXLVOFnQqy/rpy4Ws2muv9DE"
            + "Z07hdjVn3y5sc4m+pdvdRYs8/iEHXt3IBpTUJvl8LYcOFAL9Q7vN7rRwVDzq1nBPbpB3u1mtQ2H/w+x0t01OLQllXAPB7d6mYTPPdYjhfdJMekVTOrhsrN8d"
            + "grgOWQozvNI/c1IIiCAGZqM0+aGLF22jgoOheHewj6Kvi+TJ+qI1iM1tvhg+qF/du9QKu+YQIYcz/GVuzhqMPuL2OYWksKPiVvHVRfe1uumpFa9BryU+kH4o"
            + "Wp+EVxQQfhbhWp2HT+l2X3YXa24nCdUopV9pV29uf3slrwBHTCRoUs6j6IMonOAXVicYeP4poouaP6hKhQZCGTtCEkFtC51eVgNlDJdgF7RiNWoYW75dfB08"
            + "g2qcJZiwe4rTma6W2EJN3e4ufkcrRQF8Co2l3e3cD8E5EJ+jZED4JnkLn7+kF33SrkV/OmikqPnqNyh5aKzAAOJIjcgZir9d+KOxee25SQ9gl4EhjBcYlKl2"
            + "4AY50dT+LM5zW6/SpFJAHNsazi1jBmj0dLLvNnuJiAoLdZJdRxnYRJV9BSZq9KYIPxpDu8WcgmX2brRfbBKlS2ZLbfsp5MHrMPIhdfr8NczKPzdpSbZQLWX7"
            + "mL4JMJNZbvC43cx1UQ+CBEb+0X5+Q3Wocf41GPLtLFrJWKGY/TriPy2THHg5byvJodswg5kbgXzeHvxiz7N59Q6heUd0YpD1SEcOao2o4DyCJlTCQyxLcxCi"
            + "iCGKBTkWayDhl69vvo2M8BCSwP+ITMSZKESKAUR4OQR5o12uhTaV52f8AmoBHPXkYraAWlP8TumpoZmxkZ6BC8KST+6Vc7eOB5ts7uODgGi34Dm7eXYKuik6"
            + "n2Vz76E1P9mL5V9eI0kOLMCQqsRoXvqsm8geurTHzrLJLpneFYHrsmdeweZUpEmrNRmgEOnydVh3GWVwJnu8xGvMO5nJ1wj5i4zQHB8ZC61nPMOEHikpN9KU"
            + "MQ2VaBI4WctqEJLah7JxVNZH8gmRvewOQOTPfTScXRQp9H0o2pYmp3tIVbXsVkS3wjapl+eAD6OLlPpdWjwYkdFH9r9E59YBBex/nAWrjcqxSUX/pHcM96K+"
            + "eKuGKOiu2/MiLhPZhRaJy4NrrAEg/Zi1R7RQDRQjm6FIdKpE9pmjlDbDLpMfuGAflTgCQujjRh1VYEeYEz3Ls6sD65CfiHM4p+eNOK2QL6Ewh82HbdFLVZsc"
            + "UlDAX6BW1UQjV094oenrZDGx4kyGByZQjhkBdNyevnIi6bwdIBqPGHIn5n/6kXuzgeXIEAwqINo9KDqYJgQO7P6O1f34kryWKWtF2gxF22PDoIiriwU0t+1m"
            + "Fd8RDTNzXVh8YAR10xSQrjks2xhSW2hbGdP4MERU/MTVjzqDs+toFzI8sJKlgra8yU61m5t/BSWZ42syh4RsczRiX5bkY9aB7zYSbVPBfR1OqDwNA7FlL36f"
            + "3jSmK0wh01UZGc1DZHisf4Qmnn9MfxaA7WC2i060Ynr0NsJwptnhBA3ItJA+km/EJ/IhBdn/EIAR+N26AnzOZnf9pA1zdFtKvTIlDwxdFxFgxwSkUMNDFGuN"
            + "xy7VXViiPoXKp/sYn7mQ1+0uoMBVzOVRHe9mfOYg7eyI3GOB7Wqi22kjElPiUbrgPM1m6c4YBVHNvpADfXzE7T5T1TIFaTrdha3bJM0UzOgKVve8kFowEIPZ"
            + "O234MlDNNtPA5LHLnEbyLkzQ9U1pE5e6hFtconUnIUcD6VIl8vU2w7sOzd6ZNlv7GHtDwz+sZuGtA9z+wG01Z5X5Q0S1XEwjoXyAK5+SUCHXXxJf6JqhhME5"
            + "z+iNycSk5z+dhxtAm/Ygn7dnOlkoGDESnFonu0JbjoDk3E25ziisPFTqrwYZc7IOMa19aiLa39BLK/IWskc8SZMC9rCG09XEs6Ws3g1Xco5IBY/k3u36pwEb"
            + "d/dKgxPy1hTRdFM2OlADD7mmFZc8XErj4XYbBbF7TFna7X+IDV4atLxxyC7SLBn0sLQ1arRTNeySoXbHZmCE4YeX/kqjMbnK9LVWKc85NnOFOB/osNlYWcHD"
            + "HJmshVdsnLCVlCTu1/sGFIhgLUdkz5NtiUmMnsBA+TCF+GszeK1f3THXpX+kj87W9azyqyUA8uEhb2b8TynYEfxAXDabJwCgUm7Qh6cj9Qkb261Wk877n85/"
            + "jNhpXO0J6f3BnI7MUBJwpL7FwGKwMM/6Ye/pe7rTmZtCSQezdJkfSPIPCLO42b+gxeVS6AOBGkQRljbKc2BewvFkZMnT3lCdtQGyaFvwJMDNE6icJu0ehTQo"
            + "/lLXtPEAtBUI7Co+Yqu1c13IXElAjTDPCShueIRwVI2DYVC+EYUR/fZCXQSBGirEXGdPgShwK6P+n0wM6YuOUrl8GJXfwkRsdoydtFvI5gH6QMXJmTvxKSAP"
            + "pA1P2ouURBV6nRC0tLTPR6Y7gputUYUlxYJ+ZRoJJR5UL94S1SiKwTNVPKnOZWGtcyRsqU8tf6Rx71w9rdEa8ttKj854rxkig13ZQYLsorpsic1wBPDPDWrS"
            + "DHJBbY7nGU0NeA7V6eZZ+0OD9rt1QV1Es5LFumg9d/eWUrlchiU9E2wRP9in+RQtK8AC/UdNCHr3S2YQpWAQIa6r5QFtAiK0+yy2dMzUOzCZ2c3OQk23v/4+"
            + "nLZwWmuojlCKS8ZqdZtnZh7Kgr4xF0ZCQsnNqq2e9Y8+JgqEAzVHcGV6ybw7QSIbARInVfBAPwx7S/d3xlCgQmyjRb80c8DqiNui8SfPPGumBExcH0kbncui"
            + "5gs9UE2A9GbN0FpdeQqQQn9Zl3l0qojL88HpDGtCCeNZl4UxgOwahXCXNHu83bH+TQB2Fm2Muljlu7u9BQbqMobm3kODKhkzmGcrc1Km5Qs9aaOY5JEYYfEE"
            + "ZEb6xWTaMbXt7AkznqWAoozxoVjIEt0poqNJnoERZIAAIN6E+E5895KRLF081ex5nu5dwsHSIv18zOzqOZBDpofNGS4hQyoWhHyCTCKa59C642aIhNXtQvSw"
            + "aArZmfmf7dc5eHueDOgHfvv61O+IBJjOsbvaQpl5ZrjHqFwUjvX4JqMGSPEupNmqy7A+xDNP2P/R8J2BREDFb9pqswZZ3T1ygamJbUhby8lCIqjbtHwjqsAZ"
            + "EKX0lrSVOUtBj97zMYn665ASZgxNBtIoB+g8do5IeEgni0CMgtS5y7RqmAv6Ycfb+eQB1h0c0kjaWpYMd6WZcY7wQ4LYAvA7f4Mrb/Nx3aehyjxVyNBvqG+n"
            + "LoOvgP1GFXHKerHSS3rnkeJ84r1Buj6m36CTGQC3N6vB4V6Z/8y7L7laj6Ls9oSNHpQyMoxmdZ/mEI6AoYiX08v6zBzY2b0S4tlhvsRZ08fHcAAICOZl0RHJ"
            + "NIAhRxtIB/TjSuMecVMiQz2wMvMj8zMHhjt9LWC7J1yRn3QRYXZRotZ49Pmc3Y0qZl6ky8W/HZ9KcHXSi24kgckDD81T3H5fyv+vY/RrSFHhUObwyKdRC+NC"
            + "YLltRAV9bnExX0UaG7zr6RGanklyuYO1fNQbrQTzamxEcJbVLEu0zUeAWckRPecEXSjHf3EjfM1yuzT+fz1rQ6bZMCndErj993VIQf2g0JjiHt19sNp40Le7"
            + "l3w36fz1aDSbPRJjRprMk4F+eqTrezImpUNSfn9Dmz1h8huQ3mUcuq5mZQi6+iNS88H9TYXD1Wb1xqhzJYksQ9q6Z3PrCa/rHw97/TNOfIjeclrN4MNHT4Gp"
            + "XZhI6TR9OHweAAilFWpK3dqmicMhYHWf5hyJHTlnn+nfTHxiMOkYZeARzXPDPPPVXUtH+IV0lFg5MKWrjjGzackx60Ff8BS4ZvFvBBRkw+ZcEkw9GVX5BsA4"
            + "obbJGMMZjiAjaM5//NhjJU8ZMpDwE9KA4YB+QBQeaYr8VBxSu5UzEjjoie/nBpjfTrNnbxmY8/YrmeqRaV206pA+WeKCssu5tKOD7UGnDHGL1VO7h3k26XqD"
            + "bv5GnVOt9w4ixvnZdyZlMalGrr6MmRJGf8pibggo2GhPF7UYVQOkzINdZYrUEUl2a1rE882myE4A6kEGDkVYtBMA1xyWxShK+QZKcEy+Mc6aLl661KcoNNhr"
            + "9HWnUAhwlS4KAWBGRaIC0zFaPurJeB6Esjs36z8BVYQNWvbyDZqgrl/yIwKSRpQjozmojMg9ZseIMn6nPurNIWXUQj9dVBYfPIK460qtMX1K4MzfWJiCEqlS"
            + "nxbL47YC/NEJPX0u3PswpKF7UZbb7RlwvXqkeb/RRlSAd5qkdn0QNmaiguXSknNkmorHiav+pUUFAY/nfW2eqgenw8t2mZWagTPFxs8M5yQGaFlheLtgg0eE"
            + "eYG+7ZFQT/UZOA+NdYjlGdMVPmeCa++2fePPTENFbH+kl62TjmIIbMKMizlydGY5Pp7G4IT/jOqvlfdm/hVzRfi1EWrAmUok85/9Iw7h1mdlIx8obWe7Z4rI"
            + "agbwCDD1rNG0ME0o06LazC6j8V9JWllynLsMdacmVU1RGJwF1QjYywapVClXGPwuI0Cd9q9mELrNpNgRfaxnFeocPZlwhTgQzQkK2CEwO2UjExhAbL6JBE1S"
            + "wcOoiGe/X6nUzrTKfmNXDyEYS+ZVjhF7Dx6XV4ZM128zSgjSRxBucXOoE6q/0V4C8UqXfqsjUu3TZZe6dQDws7x9mhfo1FtuM0qodAD2gQuWcBNNJnfxVGYZ"
            + "9r98g+EPtNla4isdxRz06fxYddO1ZU4LD9AlaRjp7qH5KhSPDHptwhRLU1FoNTrD/X1tF+Cxze5uMnI5RMv+WQFVqUS6Yc94+dyrZa7L5hQUmJ3mSvTttJ+z"
            + "mHQRXl/3jWIKoHGX5hrqMk2PKfwS0NdTeWYxD5BtlJarldhNWkeVHliTwGycPr9qcZmPcAV0ZdwihfbMdAixWkyZa7/2ls00PVNega1wKU9Ey2NmODPFsV2/"
            + "Meb2Qs3tOepuewwDOOhGs3iNObMHGeR+pKnkypzP9pv/fzuIaKhR4Xw98Tykdt3mOTYekraZWIRoQP3bwrUhqt/8EzYmy5iyfhvb7BJ2iUdMqxg8GleXvT0S"
            + "szUfUUzoxCaXfyjvRx3NQw7sc5vN6dwcrZXc3b9/plGrNSZZznTunxm5DN0g/1+YQ/L1y/ffxLBUi3Bn3WpcC5Wjf+kgW3Yl3e3TxD2J10Bv0KVBDHR0DG81"
            + "WZtkpXGbE9Eoux5S77f5FZ7Jvz1RvsJn9pyB/ErO7cl48IldkDHPDaObcjcVgvADU1ksCShTfjmIGV/dP+MiWUnmOCHmP9NNoCuUKz94NKTl8PkTYVrPfybT"
            + "oAD37L70LnlGzWaQpPO4A/+1yU82eEbH7rmX56dG+EcjTUtaL/UOcfDKANI2GOyU2e/N92NDtwd/uYMbiWaanYmeXXD+djcCDFdrAeHqX/u60j7whVp3nK2e"
            + "fUd+e9z/9xs6baZykcWh0llWIwMi3dwpfwWH/7S7YsEKNeaS30+ZMhKBbKTPD0l06ZVu0ng+hgUg5XhSh1IaTLenkJ1rUpE7bP7uCYRnRNpLBCen5/yXI6Mz"
            + "CMrmYnZPpu2j3Vryo1rMn5+lkqJ+h7ts8pMfBPEnT4UCbcxocYqXU/Ug8YglOqPd8jTO1c+ZSQIeZa+WzD8NRhs5OrGY/iM6bZEc0Kq8RMM/7f71HGdut10x"
            + "uTdTTz0LaI08JqrCYbVO5khr6vyPWH0gU72tEj9XT1GAUOZ38TyZdjNhN34Ty8koMt3iTFI6J6YMXxNuaEFYD9T+nMojKnQIemq3YXf/QpOxLYDtk2e5lCUT"
            + "yZiORfthaw1nOIKoknA7oVPLGQ1kcCcP3TqI+wHK/It++1/aiULjTI9VmzPCrwUxY9986G7VwZlZVf3qHu3r9lHiryixT1Ve9CZYW7I6xKiKL01+8bCLgO1K"
            + "ZxA9YlgLY/fou7HoIr9DxJgyfGCfUcOjMtXxtu8dPvSbrh+dfZBh5vnQ9JFxAdaadmkRGvIrZkyXnfNDb0fCepN54NP9X61ak5/L8e88rlYnfqNNh/Q7HJzQ"
            + "xFAjhPn1N/dLevKMh4xNm53MJG7Rv4CwuS3uCZ0EJoNkDonykiMjQtx8CuJbOqONUNGrlXq8/4bi7hmbwDw3nesz8OaQouPczB0/kZiOm8kXetKPNFaj6EOj"
            + "DswyiVn2CM1EOmocobLFjO3u40/OORoPLMxsGYPAfyFG6kT/CuT5TwPR/MkMFEP9k2rJghC76gOFGYxLqq0hQ4qYqegZsKsXE9UuWNYi5c/4T06I3GIxbF4o"
            + "ZrHwYfv7kdCBLvucOP+WUNAVPYz5Qc9ZzY9zHY7g5frAydU/pIJUVYO+y5JhmPwuA/P0ejfAlv8BxATDKTB1AAA=";

    public static void main(String[] args) throws Exception {
        int playerOneWins = 0;
        for (String line : deals().split("\\R")) {
            String[] cards = line.split(" ");
            if (score(cards, 0) > score(cards, 5)) playerOneWins++;
        }
        System.out.println(playerOneWins);
    }

    private static String deals() throws Exception {
        byte[] bytes = Base64.getDecoder().decode(COMPRESSED_DEALS);
        try (GZIPInputStream input = new GZIPInputStream(new ByteArrayInputStream(bytes))) {
            return new String(input.readAllBytes(), StandardCharsets.US_ASCII);
        }
    }

    private static long score(String[] cards, int offset) {
        int[] count = new int[15];
        boolean flush = true;
        char suit = cards[offset].charAt(1);
        for (int i = offset; i < offset + 5; i++) {
            count[rank(cards[i].charAt(0))]++;
            flush &= cards[i].charAt(1) == suit;
        }
        int straight = straightHigh(count);
        if (flush && straight != 0) return pack(8, straight);

        int four = 0, three = 0, firstPair = 0, secondPair = 0;
        for (int r = 14; r >= 2; r--) {
            if (count[r] == 4) four = r;
            else if (count[r] == 3) three = r;
            else if (count[r] == 2) {
                if (firstPair == 0) firstPair = r;
                else secondPair = r;
            }
        }
        if (four != 0) return pack(7, four, highestExcluding(count, four));
        if (three != 0 && firstPair != 0) return pack(6, three, firstPair);
        if (flush) return packRanks(5, count);
        if (straight != 0) return pack(4, straight);
        if (three != 0) return packWithKickers(3, count, three);
        if (secondPair != 0) return pack(2, firstPair, secondPair,
                                         highestExcluding(count, firstPair, secondPair));
        if (firstPair != 0) return packWithKickers(1, count, firstPair);
        return packRanks(0, count);
    }

    private static int straightHigh(int[] count) {
        for (int high = 14; high >= 5; high--) {
            boolean found = true;
            for (int r = high; r > high - 5; r--) found &= count[r] == 1;
            if (found) return high;
        }
        return count[14] == 1 && count[2] == 1 && count[3] == 1
                && count[4] == 1 && count[5] == 1 ? 5 : 0;
    }

    private static long packRanks(int category, int[] count) {
        int[] values = new int[5];
        int at = 0;
        for (int r = 14; r >= 2; r--) for (int n = 0; n < count[r]; n++) values[at++] = r;
        return pack(category, values);
    }

    private static long packWithKickers(int category, int[] count, int main) {
        int[] values = new int[5];
        values[0] = main;
        int at = 1;
        for (int r = 14; r >= 2; r--) if (r != main)
            for (int n = 0; n < count[r]; n++) values[at++] = r;
        return pack(category, values);
    }

    private static int highestExcluding(int[] count, int... excluded) {
        outer: for (int r = 14; r >= 2; r--) {
            for (int value : excluded) if (r == value) continue outer;
            if (count[r] != 0) return r;
        }
        throw new IllegalArgumentException();
    }

    private static long pack(int category, int... values) {
        long result = category;
        for (int i = 0; i < 5; i++) result = (result << 4) | (i < values.length ? values[i] : 0);
        return result;
    }

    private static int rank(char card) {
        return card >= '2' && card <= '9' ? card - '0' : "TJQKA".indexOf(card) + 10;
    }
}
