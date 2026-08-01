import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.zip.GZIPInputStream;

/** Project Euler 083: Path sum: four ways. */
public class Problem083 {
    // Gzip-compressed official 80 by 80 matrix, embedded to keep this program standalone.
    private static final String DATA = "H4sIAAAAAAAC/z2dW5IkuQpE/3Mt9aG30P43NpxD9FyzO9bdlRUZISFwHIdYa+2/cd79273vv9vjb4z28j99/J29/tZc8TfbfX8nRv+75+Rfxz1/ETd/2l/7" +
            "mzfyGnePv7fu/LvR79/r8/6td/7unPkLeZW8dn5izb3/1s4/3ZcfuzHu31in/Y252995/W/uvv52RF63573dM/OS/T2+Zv7tdvKaK27eRH5rb7H+4gWfa39x" +
            "x1+cvPtou//1nl/69jx/++a13+3n783LM/T3t09+fd50/ns+1ni5ArkM8dfzufLZxv47My83W97NyEvlPYy86cZd3vy1/XJd8vfzN0Zebu6Z/8l/ui9y9e7M" +
            "C8cZ+cMz83by385qPRco/2328/3qzCfJZ73t11uu1Wh/feYO5I/ypvvhllZeY6188Lu4c7agzfaXK3j+1lh5c3zrOvnTV4uZf8115Lq5XvPmPszW8vbz8c+e" +
            "PZ88N5vnnbkkf+eMvMH85tPZ6JFP39nZfI7+F/kN+aW5DjfXMB88f+vcXLjebn5Ba/nNLHXclyvCVeY6+fv5T2flg+fC5S80nnS1XNb8+x//tCLXMh81/zRv" +
            "/gfz2C/XPGY+x4q83/xYGkpPezppYvHy++66+e8jv2retIfc7ZXrntv9YuX65iPPmbdwWy7vw2zGDMyO3cqFOjvXI+01f/jyTyufb+Wjpk3siWHc3zt5k3em" +
            "Ne1c54Nt9Xk0sMna56/v/OHJ3/iLnQ85clfyqXJBV1r938zbz43N3xj87jjn5c5szC538F7PVePLRqRRtLyn/kYuXH4kz0UeopFWnAcxd3DnT3ukUb2Ru9rH" +
            "zj8trHoGRpaX2nm08l7yYXO986cv16qfXPXc5M2G5Q21vNM1c2HXCNaDr1wcyMiPvJFm/aKdOiU9LXDvtLdccOwoly1dwqt9zX/aHO88qP1yU4ujko+xct/y" +
            "tOcS5NGaGGLLxfibR7t7uZws/cuHwzTyDOel8l/i5J3dlYd5t8kNdA5ccFOz/e7Im5o9j97lXOfhYKk2axNpizMNKrcJw0mD5No9z1Zf2Ej+j0VJJ9F62s3C" +
            "AFdu8KpbyGdM2+UXc98uy8nh58h0nEhgh3nnuTlpAL2xu/mNq5/8Zc7i7biqtEjOQ3q0fEbO9ss9X4ODwX6+vPpt+Qh+gM3u+L68qeDZ18zt6riInkuSdphH" +
            "aaUJHPZ34Wt6y23UpA6WflqeFgzijPSIZ+dCdB6sY1Yjl7pc8Obor7xUubncyoa/xDuwM8GDjDxfeSW89MUx5SMFp28+TS3vYuSZ+M2TvxstD9jO9Xx4i8tJ" +
            "vRkEWLPFE+WisThn5pr0wH9xGPfClu4kGoSn8a9WPR9mTHbvYuiY2Myf9a5DHcSIfJjIPYyeNpTbkPd30tj7fMajRizJr+v54bSyyV/zsVjRMTJwpF023B8x" +
            "B5eBN983D+08GEb3QOVRGG2xfumrHivyGn6GCDQa67U41h2vG8TCE+WK0ifnnc30C4HvuzM/8nJFdvqAXMK0D5d6YmsTZ54m0zmoLMtkaRv7qK/M/6RJpz3k" +
            "J9Lh8ex5DxeXaozEcWfI2hkV0r9sDuzD16ajyfWdBOs1ck3O5lebriXPYN7g4lyO9XJd5uncL+cjT8pt/GEQtzj0kw17uHuWPM/awbqwiJ1/OzjJe/LsDxzn" +
            "nPmnyU3tgT/merl1ab53cY1cYn794cDPY/2CmJJxg1jFJwAJA6O53Eo6gLxtIhkg4W0iN6avv3xE08NGd25lL87z3YaGYKM4PxhPOv3cfFDKybXomgGBK0FL" +
            "nqKXhv1yZXKf8846D84e50McIE1awcb/+9W5vH3nf/Jc5XexiTMXdmA1+Tjjtwg5aQS4RzaW4JDPT8DND+cjXsBBOhRM9rBs6QEu9oDtsfkr//qGSCt30IXx" +
            "CB3CSH47H3y5VXHzxxysPEgTW86rJETKH+av9AIwHR+Xp5aV6ATTBwrYgKP8QW5lGmM+Z+KMI3aZE1y1+P1cynSq/KBhqZwZ7HM/7Cg/u3A0aRc4we5W4BgI" +
            "7S3RxuUwnuCIt1zZPABRJ/6yR5zjAFB4qgI4lFuUseDgH3FUuS8CrYEZ5UkbLooH73iRxS7iYg8fwTfgLxIs5pZlOPvt/IrJ3zauKHcWZDfc7DxkBpTIa64B" +
            "IhkPWIMHnXjVNAHMg//nRSboEq+/03pZMWIWp2yfWWuSNgUamgT/zZ6wnuMBDf2e3P0MA7nbHQeVFxk7N3oQUfPg8rC49tUA0e2LW1HgoRk8cisPJpQrmT90" +
            "dYVlBHe+pgG/iFmLLzztcN7zYTt2q3fdGEDDH0d9IZ55uXT4XwIyF8dlrYs5YR0ZRrA3lh6PnK4CYJ+WkN7XmDkIWJxZXSCn6D2sE+fy0pTS1ZxfGjRHOC+w" +
            "+V2hdcJjvSzgEDwM/MsgmP76EHJysRZ7PwYhhzN6wde5zaCbNLkJTOyEBEBcRD5mLdME6abpRK4kyByPmDZpYA6cYfrt9n09qUemL6Dt9FQTDLSx58VRyq/h" +
            "aD5CowHQwHHLU7sanLoJcE1nyroQaUBj/KFzaDoHdGR6kRun/8HdppkC4nP7gAPAJlY/0Vo+8mbzjAlnuV15A0SdxL2AzMmWcT4PUZZDAjDDDwauPwEgPgjA" +
            "1dlygV3nPK74TZz7IhlIiHvxmblYuKA9iMXHfa/MTCsaFddGN8cDh4DJE7Q0diYDDt9wMSAgvfb4LvcmzOa7Dp6BpMxtGoCi9HQEXaD783h2DxCQKFxL0ihg" +
            "Pm4YV3QBwCdzAswz7YSP8DUT38+65QrkjS8CF5gk/33io3G+B5CKE7u4HmDBJPWLhLlYct75zI+k1wGR5q/dIfCYpo1EyCU8B3T4hGnnGRW0CdKXSgzT5oDL" +
            "bnY+zBmX09AwX1KHQUa08q8dWJ2G3H4HZJWJHht/xYlArox5T/RMIsyZTJdIPgECXqSuGXMJYICU/KeONT62IJ6rcYwPHQCTXj5IUzZoiiSEC5HiBfnH5a+X" +
            "gNczWnLe8iDn+cnNAHnjQj8D5/RutqZhY+4x4BIAP55+MO2arQkyTv4fZMib+JQxh807wPQ8lddIdUmYb37NHsR+rpnW9Q85HgAvj7aBGwAunQ87s4p2IOMk" +
            "8iaunRjKBhjhBnMHE+LkrurMyLx342AAjiPIhSaQhy8AtaQ1j9/HNAyQM5BkAyAXiRbO5gG9BqZIMpGGktudV+dq5i9pz0C0PNsAf7YWZ7CJCnnGAqsC9ZmG" +
            "kY2QzLNBFXwuN7y5E76nMv9V6X/6i5Urn3i98zkMrPnEeVO5bOQ1+Qmz48bHAgiNLR+yCHLTzP0Il2nkeSqWKcjFRz+MIcNAHgDi7h3g+z3r1OcyL/LsvH0S" +
            "9kwLsLtcM/JAkpdEXITjaR6H1+Q/V0+ahy3IE8y9MuLn3crQ4FLJ7XsIsQoa552SEZpmk43/1pLHyMvNXb92vgsQRDLVZA+ON/dgkQhlBlvypNwnqApy9IPD" +
            "E66Rz5dD3rATPHrmj2QfpCXiX5b5uoibxOLhzHEvjyc1gskKEEsnvgWeIZbucOLHCfQsyFmCO9gUYuEw8x8BwOShOIJPRBnAZE41yfsgNhwyr5gJ0BL04aOJ" +
            "WfOV/SU2JnPDlWIcmeZgA+lJiBlb1NAw106sxUG3XLWbh5EsNEinCQR4PNOrJU2yAcTBfnAQrtBw4VgNNa39OKmBEwycVTfCegbucivZHW5Eeg97HYSfbfJH" +
            "jN14qyuFQLyXY4KDMUfF35Ee9G5MhMI5pme5HhPPCdaOKCB4iIdED/EGWR14d9eNn2bMd/O93RuPjJwUwYQ14MMCNMv9BVZhdrBArRO8kdsu4GXf+TLhOanR" +
            "FIovgxkLBCSc0Bq5cDhxcl/WcAcoGsrh8eUJAhaJG6EIxJwBLA8XO7cI3xt0B2OTpwOih+wKyzmBR4JciIO3xVOcO361bBsXzaY9SEFIyYXzPLJN5N4hF0Ds" +
            "2VChj1wiiJ/vksFAPm5zfuD1kfTgth/H9nK94AESc8j5dVa/4cdZ7jw0GijJOJcgizU9BEJ1IG8ja2Yt8glgI6BfQAb5aw1rww9cLJVUAeLkHgI0CeUBaSUM" +
            "9V5IPp80az6qaIFzQsZJjgkBOwOIBeq8xFQ4gkOwv3IvGWXyrgIk20lNwTAmCw2HkTYHwJLFY+n5ME+44KAE+wZ7csIBOAem4xF/UjNArhDHv+u5H8UWbVlN" +
            "LDl3AxoLzCWYgSfUIWxO/8PdPO9hBhykCD//dNk6MZhU8CasTXkrF9PPDaIMETgTLZfG44KdwgylS8wQ+IiNuf0NagKiJ0isWM8OSTfJGSt5lC5glfCFm/xr" +
            "g6jzt0AH+dNBOrbx34uPDFxQ2nUrVmTAQnOXiYDkdIhMJELgUczd283tGh8lWo/ADzlBZAtTIgo76pCX8oQSG0MgQDySsl1YHSb3oDQ4OrPJrw48Ud4xUQ1K" +
            "eHTiqAmDLA+xVSBshnGgxSsuQDyICAakm/ANR2bCdaFFDujwsBPp37GahlsmbYTzeIenesOc8OGu8DHQ7TruoExh+QLku+UEWQ+fGdh/dEWNDZ4gJlhRorLL" +
            "fKas+uE/+VskNR2ug9RuszCXhe/c2oJ9pPwwMViiMz+Cqk3EBjaHTcd+AnTswxHG8nwCBfHm+bD5Sw/gAGfxOA0whcPjJGfYRJawsGxGnoQM0TiWS2xNG4ed" +
            "XuWPD5RohIcw73nifPNoQ1lATOsMDmfymqo0GJRwO8iRcNygwoCW26666AnuoMh8UgXwKGB3WtDA4vI6JOaSeYRm4hWOPOBnYX0fpEPeGmkQxL/c+DARzCuS" +
            "rUwY8PskhymbpO+BmYXJkUuH9dOJkUfg53KrFmWidKUcSgD9kQiTNjtDOMfZotrAVhP8MixNTGnD2LCokHA8yvMRgQTdako+ygMIjgN/N+UbAby4RkiA9H/3" +
            "t/EEYNbLib9s/SX9muRrGaGI8BjJBa1vmVQSdgIb9vLwCW+KfvFTeDbs6botJL0DmmPgj83Bj9wq+GSIubHdLr5vxV5yGqZ8P8nCKpJyQhpWnN+kpSzNxZqG" +
            "WUQnbstE+KsbC8NgrGON848gzdUjpjWAOPwJRZGNQ+7sg/E93UHa1BZtQkNh8rnMoCAIexbrNd3dNmRbGuSpOJ+gyH0mDj7ad66acZMiI39dxlcc09MjgggW" +
            "Ncs14vw68asvyjKZDBI0WtFsi+MZhISEfY+Ef1SAupTRDmfokWAO6OWFM1nPODSJdRwVdsI0IHBTuqS0scaRys+BkA/rDxsR8vdPqg+PcTABQOkFXJnUgOgO" +
            "JypBIjgAYMQidbI4k7J0a4ALMkFcGjxyGmN7Ba/m8HAOIsk/OiisBxCjLfrqoIHqJCsbuBisneWPjtHlHhEXKWJCBkxZAjz6hdkc5EhRgAtvQjJySeMAyt3s" +
            "8kH84M3SQvHoVrVy8xMW/TpxYMnyvM0C9Y9ntTDCt1ivO0PqOkhhDugBpqgPCr6EC56ya24cPipjD7difGNpYpuekw14tKgk3G1VEAJqS5dNShFbngbcxE2P" +
            "9P4WuackIQ5ns8AHzi9N+5S7vQIJixeejc4ZBBFu4XAb4MhntMyv3R7aDfrLNR+cJZCG1RxqGl0fQxV2hDn8qmLkgnkvXNZypV/lOpSVHtW9a3UXMgFGmHgC" +
            "s5W/uSr3I4gdOIkpF1CcBsRaft8vrlVCnC8wd+YCp3exhogXuYVX0721qgk9EbFJTtfbki6RK2ZmOyEyhW9KCpAaUCO3iN+Mn2m64Y4NcRwEkaiCQqHgioJK" +
            "ADR3+Mx4cCBax2If9GH6EHxSlexH1SzE35dT3fFJC7ySCcWHP+q0XGsTD8OBOyZ6TGJXfxk58susPsiWIznYVLh5cNybxVYA7AWQ5XKyyVyObzxhVRndg5T3" +
            "I7dccFxB/tYhQUHtRJolSMADEbKI3Is4l256/27ztmCBHl6A1VnQFQSCuYvR4pzsZXxJt27ZCobymkcC+pY49Et5dR2sPqDoEv82Xr1jPxvfNkxsJ3bGGm38" +
            "bEYlyozDGkaur/Sizla6Pr4qVlCxfxIQBA4I7HRxMuPWXHAyhDFwy3jwGrCbQOlZVWdq7tD+1v82EGDCHfYuUu3ny9FYn90QJsBRX+uinNBFMnmkaTySxPTB" +
            "GXwUgiae7uE+B3cxecRFEehKaMI0yykNUyPyDB9rnV8RCVCD0UWLcDVEO4uYAU+bSQ9rC9FMFUQPDpEBWKt43LAL4uwu4HdJICw1QT6G/O2itEiJg8hHlJuy" +
            "D+SkD7puEMC39D7pbcfnWVUSrF2QWCfLEB0B3yjIPykUSgr7SBuagxISCFwYDYoGwwBl/0ESWGYI1Om3bDUglwZQ/wYfLdhOKn3leU1P8Jz+hvBSYgHy4LAK" +
            "YbFAR1zMFw8ApxNlBHN/sgMqynlu/QiFP2J93z9VB1X/JNGWT5oWWtjwAybL9Qf9UN6AdrrokXCkHesKKIgHq0odb7BgHaDeAfUDPDrJ3TPPGlWmvULWJ73M" +
            "4YBFsUovJ04mwXOAvoOqqrT/JmrBP01r1yGq5YsQqlDVWvBUe6jJ4aZEDgSgZ4RogrmhaAKsOxTk3Crsi0kORWnPn36e/GYTg69KDmh3wkGedPRVbBvHtynK" +
            "gi7BzhIVKI+ZRagv6InACAYZaF4MAgkbw3dV5G9dvzl/1iZF9scKGHGDRGVzOp7BHvYsL5sWbnJA5KAszZk33BLiKXVNQNRsFgJxCtwZsQ8nMpo8PMeeKgtU" +
            "C5+DxwyQ3uvWKRdnHmXC+1eAlLG/2up2r7kg4SV9Nn4Av4xvyNPkAnG9BPKLRe44nIzFFEziIzg6jOW91IdAS3eLWJdV0U9Ts7Hpsv1So2CYMsm45M6hVIjV" +
            "Ec6B04HNl0pOIIVzBWSO4WHkh/V5uNeLUmGwThn+ECad8zuEpaVYCfN4/DqGeIkhqgcGSCfBIUgU/06867ir5Tp/eQ6/fBDqLOuZq32U9GZRSkNCGXQ0H1b8" +
            "tFgKnLpqQYiWtFBw8eWMLWK4+JU1YqUzPFpT4OobYQYOp5X/pIpTJR/sbVPoEbnrDY5BlOPLo1hwIB5wuEIhls8n0jyYBiEI7w0054RuIn5AO08dBjVaCdtu" +
            "xs/hGECGDX+xOAQPCz/QmsdqiI76WEEkNVNGA5WUIPWR+9xe7JnVXQq7wwBJXeDIg4GYcDFt8h8x2FBHA5VhiAHhIZiQzLlBFERvtnimTepqzv9QBAQsQ2+j" +
            "MoVemaFnnVA3eS7qNVLQVGWWsknEhJzsazrNeUbSBOt6qlxAstNI/sn7cp1Wlw/mct4N8Z5qlfXUBVtjtnrwCwtcablpEWmGEjOPEvqJexWQNRkO1CHQeFg8" +
            "xf8HWTlAj4EhXiKBJEve2ipCIyaPjrYOesJsrsNvdaShecrab4cVODI8+X8sLIaui20HdlwWbFXeC8cBiIPuuiyPFfA7xIiEV6LdhpQT91qssuKgTi6Txltc" +
            "8CU6mXUSNghEC7YtIK9V9BBXRjHssE6EhkA9dPAheQkShwePBlbsSCot827D+SLezaKj46N+oIhxcAWWxpGp4iC3TnmBA7fGV2J+HL2FL5nwd3n6SVE83oh2" +
            "JXhw3qeAimoX+Anc8Sv9DE4bdVhJWU5xL0CatOOHJ1rd+jn1tUwx0x2NWVV360cA9kXqGwS1TjBN9KDYFoEXNo+5WLq/cAAhE0xcfnjP0/RPJpWPtboy9rA0" +
            "UbhqrH9qhMR9iA5IHpdxo5VC5SCPm5zNTmZ4sWqUbHeL/cw6ojQ0Cz9xKIA/K+u4TCqURwkS9witXQKpHkUGHcsLiCoeASp3TEhSWC9NFZdABYNawSYVembW" +
            "sosPe5dZBtTP0hhQAAdCr241kTwSrY5+TJ0xgEztkNEHpBZbqS63umfnMLDb2BXYb1A8eVBgh+JLSPCr6OoE85IyAm7ULVzcKmXNABsXmY++8nGzR9BMEqQ+" +
            "9lANCFICz5Ql0S25AksyKASrzxZYmDyG8fsemSqMExMgQxlU4pT2BhEzwkJZ84vA2F2WK8phPSGHKqxAj8eKTgDGuZopHhT0tp4plQJjlokDNiHsJ27tmPao" +
            "W+UoNRHBURBKGkFJjgLUgSHLHAULnf+8zzaCUTEIJcnnY3yunmY1kxlkie/XCai9ZJdSogiRWS7SC5PjXKNX0pGl5prkZQ5ln2Qh4gaEiqvwOeprCCgkFM8i" +
            "L+FpKU9E+fSIsbv4bb0KUZfEl9CwIIdWRdW7yr6HwlK4qmBNLkZzzWCe1WAzDxZsWaJrZimsMB5Jd4RoCReztl4DToCKqRci1+JUBzb+MIzZZK8xXlh1bjds" +
            "NKi6MxHOYxeVr+Jp8K5iEvWQMJIw54rhcZ0ZQtSaIyKSPdmuwPm4vXHeLyMCx06qj80DDpiJHJW5AOYNmRxFrqBToLARyMJDtrZVvSW/R3rkSJn/oxzm/ERC" +
            "w2tCYKoqmg16k+rTJXYWM86TzKleyEo+B+aOojte18ddSlna3SpSfiopBjJLjF5c4ZCBPSV2JBYOitePCN63QjhVJ92UfRV1cikaPwKJdnKJdEOUshUQY2YS" +
            "xpYCAaOIwQ+6AHXOFtpQZ4Q1TnYSpmU/PoXvtIp+sb8RWjO+jMQkkX/7LUDBQEx3BiEfLdNFN27l9ZD9dzDCUODTdesyh2RlTwHILSVLB4AGpld6H2LaVJWX" +
            "RwNLukeBMwFBU4BzMosGV21BPITBUPKHIlRRrDqJczRyq55LOXtGJc88RfVglc/WfxHMjj8gjYAoukOlFBkwuu8guizAruI1dbqeKIRSahd2q0Szl8Z5GGdI" +
            "1r5c5pTZ5g+BcUvBKcH3VVNDJx9XO7WK5VQOAlAGDeQZJIpxr9tiABWgGO8XUAurqbsjlcUtuWwdxL+vR4zzi10FGHUNjhYg5yr7hRjv1K6uUmC5OrYj4zI2" +
            "yyFZ2BVRoeBie7u6iS6AbpCf2gWj3uwAJ3IbOLQcvFcUBU7dMnUYkXWZNJJIPP2rYnT1PiDYCPXTgWAclwvclUQ6ePrRNRLE4ba4SJGAAMhPrISwv48C3pMX" +
            "FzLOyqmDJOXyxa8p6gFe0rhiAU+hz6GiLq0zp1nx4olI2jBYA5u6d5KfkMpNcPUbpAY4xuI2NWKroluVBZAXiR7SlWEeCPmzhLP9H4GiFOWoQYQPzrggJU+p" +
            "8Eg3Glhv0WOdfqsA65m4hL0XV8k2ySuJ4Ua2sWUI2UPqgA8KNRDnLY6RoviLFr+jZSlSgDJnpn6SN5ChZxeXQCZ4Tun1SBnAYPjYCyadVNYeRHvnwkuT5fE2" +
            "xY5LiJkAKaJ+3utjl0BUxvqvjCQ+lpDWoMhLJ3lp5Q+V74inhuVJdWLQUdgJnjxh7c9S97ACr/AZISvYK9QVgHObsAseVJOyR42nPdeaFmIRiOqLreHUci+U" +
            "VVJ7oMGHQprVCXuyiBVkZK7o+NrY8MxmAlOxJPB64WUn1FngW89z4QZqIYkTytBDapyVooVjSB7gW3d5aNt2cPfIWCY64H6VAdmsAzeP0oC041ikQXxxJqyL" +
            "BR4ALB45umrrwuOtTuvA8i7g7iH8CWpZgzqYekVb17ryPyJS2NRQ+ePHn11UOWbY+fz9Z11jKv6+xSdKj0IXNfXyqmrw2LbY4HAlJDH9DnidiB4eTrgTIxfe" +
            "YzwxN7GTYIg+u1MaEO0u3L4dXhbmg4x1k7Hea2SG/ENo98hmplIGwNGhiLWxehQIYxhZth2JICf+ZIiniBjPA2zj00CLTIcBIaWqbUDLYwECk1O3bsNBd8Wt" +
            "saNvJmu2wUZ2/Whf4FXSjU7fR0zbDzlD7PgEm+afECsu/ZuloFFLHlQfBvDkmR1CiA4yWw5L/kvLZI6jgDJyc5GSr3YlvFPVEHaC2ZCRkE0FlbBOAh0l8I9V" +
            "2c8WOeMOY9kw1wVa+j+aDoh/OCIyMi3EnjkTc3VJuA9bAT2/E8nMpNFvgq2nGbuEE9rJ+WRrXy8/XtJTDsUCcqrM2xat0XV0ayhPiEyzCTBpgXUflaxj1yuO" +
            "faGj2xy8YzLYtroKfB2ePE5JapDqBrXSjuJUQdZRnzdNq1HYHn+J6h7se9u3SiweLzu8ph2APHCTLD/7N6E7rfeWWp7EasjHPKmkh4zoE2dJGQ4cyHtSLGED" +
            "J/VioEMRfpLQ5AfwmoT7ZneamjaqU2CWg4qJLk/zbbGfuMOKxJLTkw9pJRIPkn4LBO+pqEbXZrvTkQuyX5MqNkepK125ClStsY8oGmNy0o/tS2DJDtORl+PD" +
            "pIEqmP1IsxaMpVIcWoifEoahL0SeA146JUe5xlHWnNwPltDGHfHsIrmlHj448yZgFkHJO4OGg01yfp6dBPv8NhSDZa3ZC3B8lZ8F4zNtYqXj5BWHQBNmt4sE" +
            "ttbuJWTOD1y5IQCeDZN3V0AY3YoPBWHojRKA4i8CZYBVi4tzPFWmRmZlYYbvOQjqJksxgcqma5lhU2bQ8sHxPJXNdvk9SLRYXw7M6WJaEjNKbhsQei2qWFqy" +
            "AsbReXSpbrq8n2zdIaU2IYGeXKQxHSjSKWrMY0Mp+/Dka5ZSHNwY51RgEmo1ccXRSrNXBVUlPksZtu1Uu5Ryx745kmOaqH9yWpJ6j60Sdaq8V4t9CT7vVpsS" +
            "q1+is1EY+1SnqnEIQUhV4/TDyjyrF/PT78vVlUCHUq1d6+RydgxvbDUoehmu5i6UCQnP8swqZgGUMFPFoV0YRmWOtTTvLheApMXaxZ7FURA4tp1nBIdpe1Md" +
            "m2CNu/JE1n0aSFqRsyWsfPoM3EOzwXVVA+xWfX1V/onB2SZO1KJUYv04oNA75Pd7qpQAK7s8YiBXPeTY6vMQSP7kC5UcP/XIwqMtHEFWSdD51MiXjHpeteMq" +
            "1y0ksr3NchA79lrxq4P+pZJZLtPBKErLyqn8V6gQtKTUJdjw+DZ4g+pKw0StROqGkD9J9yZLfXHI89VMAcYJUAwjqA9TQOKBYgovcBXGGWlDSR19JWGHUdFR" +
            "nZWc9L3D4gIbF6U+zyD2dJ6NJeyfBQo1R3LTxkfgH7tuApTHA9IjTjF75qBfD6KhHn+0rHVTjUCpQyX8Fwi6JJlsDUQkY+/aOGZRxJFdvS2jiHtxvo25eNjm" +
            "oAPIvKHnxGBYr1ugdQz62DkzN2poApEIbWSYZzf8O+gf9sBiGXTGhHh4zWYls39gGfgZqh7ftGngs3OBLCCUKDUhl0psBgQISjwXu1p4v2T4YeeGrjtrMIMt" +
            "lkSZTcJTOgUUHA9QFaDN+4x0OJytyo2/0s+5iGQqaxYOo4sxZtU+YlmZg1ZDT6yyWjGVZT/RaTqonz3uB4/2LD2b+FI7VJB3QzVSv5XpTulnx0dU70BYlseR" +
            "obeDK1MeSR64lHB0h0Tc6uM+aERQezniQAFgh5HokGZPfbxJqkJhqkkh8AJSH8pk6kDs5pkSBvBftvOLCA9LFWoJkcY/6ouDSRtzSDfDA+LUbHA7r0Q9S8ao" +
            "iWqBxy4B2dpVBG/yeHUqqGmY0LBtnFFVZ8MyOB68uxFBHDJN718tWS71qORoSH8ggAioimX5xVkkTaFhu79Yppp09NIaeW3vIvXZwxhmj9z2oJV8PewLJy5q" +
            "Km9YxeEkYaKWzQhyb6qLQLFHMrIdkwIv96oiCvrGb0Iw4pi6jlapJ2K7BQsd0FsP1IBa+to1AYtmQxI7an9LWee1ZL/VT5ECTrsRaIiy/xEHXjkxD2QdC7XB" +
            "fZYkROt2JnJfwwSa1IBkkCz1wQxs2Y+natgA3m91SG4yyhiqf6YUEp9VIfpluSo6cg12mcvGVx9xFRemtvoLAyONN+rBnAwQJB+DdO1ZL9ymUboKTuBUAkHR" +
            "oFUHm5Me3te3vIyfEFL2jUjfADtNN80erSPTn22lTi2eNBy5hAV5J9S0XufZbNfOrIs4Vi2RYsMQolwTA3hVyL9HOeDw6BMYXyTvUwxg7YyoL/rCpV1ykREk" +
            "hGSLrwtAlhXVwl1b9gmU+BCEvC24vZX3EMO6/b3MpzBnuKWqWqqSADqHmSZ2ES6AxbF+7i+XiPYoHNm/Jzdidcp8ORRiLSGmaRE8A5G7V1Q1EQZW4R3+NcXc" +
            "p9OSfrTkd3WSpxoBjw24eDTRnjUg0u2OSHvc+GQBnR8MFC3LBJGz32mVf8CHTgkyaKB5lG4PqKCTkdlZeu1PJ0IobMA7IxWgUHxtfFKcTI3pX6kCjRDZFQ6/" +
            "qDebFSz6WDMgSKv2huJiqzY0xoL8mQpTyacdnHCgPe0hVf3xXlXOqoLwbEQ9jh9qlXXfbougc4V0fOc3l72opCF+EmpsDEMe8z5UZehTr0M7WikGQ8EmO2sr" +
            "y2WJZhFAwxyDzID9oxrNIg478FQCWJ/lQA8SxmNt34pEFWdf9+egSaKjRX7hPGH5OjjiGNWBoNueYzJRRNkQS9hJf9Vg6jwZm3FH1TxnKIrHApd9TFCrALwH" +
            "M2+tqyN2KjmU6vFj8++0K0r56zIkqkQoJqkW3ulPt6JFMbuQYtBJ70jsAjGQdE+Iqeiq+RR2zPHroNZFfLLJflrvgNyctso0RZ1YIWTDpFjwWJxluwiw55CM" +
            "zqFVIzU680PvlzZEm8kP29O7VXhCS+mvqYJwVO0v3TjiW30f69boH2dMqbp/0HoXerRDXqi5uj6W0maS71MzDT4Bz7Qp2E6K8H4qSuH69yu5xQGNHhzuLIm+" +
            "kt3Wi798cM7vKP+BLqM2N6GqbLm+alodaULxUh2a4jqVDbb9D0G6DQkHoWMUtO6VBtm3spvsJImBEz12/92hYyDmKQING/Ko1BPi37M+tURz9AJJeOvQGUMQ" +
            "av+23GmTIqGM/qo38ZK+gPlL3mUDvC0vChVtbw9HYOjx6OhSv42SfOOoBqrzKb5RFQZhvWhc0HQtDy45mqdkjgQxlJF0UkKuYiHe3E6eVfEjZYUt9V/dR8DG" +
            "ruZwVLVvQaK/6SA0ysd2eSrWdRYAeu0JeTuAO12VwVOMbM41iT8cbMyM7haHGR01b/Bkq/rznedCvmetSj1y/wlM7aAfEJnb3h3iu/IA+AhFT3Z4HUeDRNWb" +
            "pmVgHJlBo6Md31OnYyZrdxuozXzuOSsMv+1sNZZ64jSChGVoU8Rmh+CEw81wSLQuLthzMy5YuGjqEKVuQN9UeLeq2ZKdkAaCMjkS9q7DDlh3c9ZWDc0AWC3S" +
            "+Yy61K7h7paTQDCXAkaql0naNmUBElM96u0qsprBHoU4rhmqSXH5dNgMYcDmEXLKQMCnEkTdwlX+3Jw9Nh2WAlXf2696f+jukqKosTfUWGmYYDGWVS5DNnjc" +
            "TMa2OHyS0lHYuPOVRHEoIEWGH9g8DAzr5W72N/btTefwqQFqX0o0gDCHpP9Qi3BCzhSNSuS0ItiDBV1LmzWq4HfggTZNeXsWVbS+SR3DqQj7VPvDUHSkZt+A" +
            "TeI/bnWEzY/7Zo1EMdeeePNrAtJUMYUoEh+JFQy6Op/JkVVaQCnfZ0fIs96Jm1Iosu1/A6JbweySJcZmzPRBtQ5bEWw2xVdBgr5mfRMhPNTClrMluYc1DfJU" +
            "yng1C8um/R4KkXE1DhwAOilwGWEbod6+OfHEiPi5EZsY8V4c5+Lzq3gyC1tVQSQU1PM3QgDdNIvkvyA0U5S8/mzVo6sW95UcRoQDMa1me6ibP4oMRI9OdVP4" +
            "LJCen255qxo3SuBvxFRqLJ8duNSAVSR1O49UUoHqkaMhZeg45t5tDqEaiA5uaNxDIILrbLfkXqPmGfT162YKkE0XUDS3Ze/3r+qobB+X4ZotwPBCJSbDqEhi" +
            "0FcItRuAhfFssxG8UqQj+DzZwBCiyM+yrRiZkzT+TT/51wzSC9ejWACDhxIZ/op64PAbk891WF75iMNADBVDVDwezOpDID+RgFq1R/BR4Ia5C81RiD5MzUh6" +
            "10xFtgkfrHwKzxAq9XgYhxJV/wkgY9so2M2uP02+kthHlSTjSa/+le6EHaj4+e+ULWusNxxKiE81Y+3n163qCA7sRJcClMSqKVT/JHqkaXAns1dbFYXiT51k" +
            "5rLb+J83AiLhi95Wz9G++QqPwFZ6EEx5GVLqgLxeAn9bdxRtLEoX1rcv+jUHI9RYtbAxmvYRalJRhYghQh82JQP9lQHXDCQKmtKh8odw9JSNnU0wLUc5OdM5" +
            "mWTVW/kD60l9w1PFnjyF93CpgM4Of7OvhXNHEzq4zHSzf+WmDfiWbR7qHsHAZg7TkU/Wy/XXI341/4dQFDa3jFVSj6AgQw+GzRTT0QeMMbF8sewBtj/SgQ1Q" +
            "NeR8kom9i4WbeIbD90+Nqv7yod7dIlGc3njORlEDT9MrKjL1KVRtHG0Uxzk9BLxu22ON6rOiTPZuR+f/+igyjG51zFk3TkWoPq4jd4r7//rbd42nWE9LcXgI" +
            "EHPYNcGJYucc1yXMWU/uhvTWTESqg/F4XQEOifURDjVbgbkJBWvPeX6k7w4SY5Fty1Fw/9Hq7f6u2hKC4thS18Al+5aWE+Oc1xY1B2k5MyMcakl7BmL0aWuL" +
            "Qy0Uxobd+fCax15nzppjktRdcOCc7HOBDpZwLHMNWnYcXbSO08mUDzS1UNaKonoanAi7jzwzWjwcmHM2CNekrTa9W9aMU1B1yGN/C5KmSiLF2ec8K4CoUrM6" +
            "TElAJ8cO5/pIjBXKO12xGO4AHBBm+f0b6tedqWXJXHm7cumlI6APvUo/o0bDSmspYbXg2cfPoX+SrSonAYDxHO0Bu09+6HguB69w8jDscLgANN1itOiVB5wK" +
            "Sjm+cruOrio/Gt94PojPTtzZsMqn+qSB3ZQSXlS7DmPz1OET+mvmy6inOGGNBaxoL4tdZVdtw/ymeV0Ove77OpOOsDpgKu6y15CiyNhf2yIoEHpYZ34B6VNa" +
            "VPUt3PdGT3eodksLQSLmt1PIIJnzeFnmcmVZ1EdMW4oxcBuOmeBJS4LNFk7LAriQdWqOxGSUzEJcZBmjU/CeoP0Nx7SewyQM+CaFjrZ76un0ohLEq3SlD3LB" +
            "loGzlY46g0DtAnCfsKLO6sCxqy67IKrFZJkh/6OyyumExI/AE3VlznJL2MyrjryagIz9YyLTFus3quLnoFOFixe4smz3BpyFU65orhAziTW/sbKquihRwZiy" +
            "VyUp5Suq6ZD/3OHU3fgmSgtYZU4O/nk6BLU0lMuRdTWneBWwpBJUg2U4KE1Fg1gMaorMwki+x3jplRy6tZxFsouBDGKu7aJjODB2OfxOBrXVPTqhV3IPj7rs" +
            "lSk23DmeyLYqb7hbJtRBVWFDTHdexJTMnrYGzlEFevXxWwht6bF6Z5yYOqqAFFdZv5OKoUlsm7bsQQVbWO1wjcWUwAN0neaMaE8upuR8mzAp4EYkp8W/CD0P" +
            "g1OPJT087HbuL0o2R3we1AlfpxBEOELHQXFvgCY2QmmKB1WW7bI2ZsAI22xguuqOlH4oTQrTz2FT/O92e2yd9QjFIwYC+tinH4Zpbgz+yBzpqCuhxEHAFzRX" +
            "8+B2bAUxABqvV/8zjox6uLPM1jdelHIFaBy7c1qC/chCO8rfso0CecfWdpfJwTxFrcNrsjuCR3tOdUb0Hj6oBxNs82K9v5S+MMgZEjagKJYifzj27iugKr6a" +
            "hjPkgPNagrKaoLZNxgNpCxUVJ1xFvG/gytPXzlViSQc7hVMiHFgDoHWkHXm4uHxY7kIfgTf8HfJAK4eOZbDWY+HiWasjX6h5SORRWy4BAlZHO5Xwr5rQOhQy" +
            "OkwQ4odgYa2rRgxU1rTfV9uyM3HyRdO+jK7qAQIzPvhgV7gDOpxMYevJUSZhbRl+Y5UiYMk/YzWk/kw5ms7LNDOWtaCgpYBi0j1mheUOcYpzGYn1zwlJlMCa" +
            "Y57M0gFxJDX6aDvSlH0hZLS+exyUIlimc2nWEFmc2XRUzPkGKBx75raUAeGj1wgjK9hj9Z91pAPc6LD94aB4dDM1LrZchb189KALSD8BxGNkZ1eB68ptHZZt" +
            "oWa/jiLFDVH7AIksyT2kjwONj6PChFuKXvd27qst4td6PPpZ6tdc3XFujrt9T5LGgWqlpAN7ggMcbm2Nw34WD91Qpg1ARGMsdHJmL+TN2xa6JVl3kbcjFIT8" +
            "G8wAyTdC/H+i4naHtLEb4Nmrq9wMZTRtutLSVolf6VGUW9nYXYPdbBUm0LGNzuNU25YA+/dAyBM/bl2ipvo4gxtFhYO31vXKyiq4snP3aDiebrAtIUL9p4rW" +
            "sc/UEQCq4BlzWVZzH0mYrvUDO21QxnujBAnnr4An1UvPj3Lm7vonXTxKExRq3c/uOe6PoYROHN1AjYpctpUA+u0gO+SD28EH1fKIKMbEinT7wgdZcvwEPQ7/" +
            "nK2e69rTgYd0TJniZndyVWmbsIBLxecr5cWXdcemjSKNP22hsrkZ/642CSEbzVyegf2bzrpGE9E/RRKOYBn7SQH+DXcpnYHy+ac6yym32xcRcBKE96+os+qu" +
            "cqjgsax4aoimtdgxSsa2azwdmVNc56yur0gRyonNypV18dnrCDSLVq5pDS+Dj7KahXPc4t5dyVyvPll58HqFBSm7FL/99PE1oCp8ncV/otYo5SuZ83XEgR6Q" +
            "ZMGh1qoFTXGqBwZUobch38DHOkNgCVKnRbn69aXunU4wtvw6XI4Shu/kWOnjfzacO7zcIL6t5nd7N2w6aLU9y3crOFAIN3JBhIfDuG+9dOHPLE+O0anItguo" +
            "cp8S6H1Xg/VRK0x6X1OutirQkivyhVB3p97eIIjaxXid6WhUyNswQ7fiJOBnaRE1m3rbnytVI8+l9n/aKX0/suse94jpPMOAWM3UvrvklCJklgt1VoqDfH1P" +
            "C2HboFZ9qaaFRNFZw6CXfAjUZtiKfR2r7EFhR226VyHrgOxml329qsIZFD/RTCcUX+Mxut9jX11T1Lp3eaHeJdKsrpGdmEoMO05s2qD0FZZtpRGcpsKlTAut" +
            "D+JITCoe/s0NOeJOp5wUbecAS1SyTSzBMbHfArs8NhpuX3pRXfc1crSXynE74MiSA7axq7A6bf1zsGRNdN7Wpyy3zHpBRACUtIVBfKz+BIfhUvht40tOq6vh" +
            "2pZ6xY29eEbHlUymoFtDn9s9XraE7aozbPm/Yb1WBhjXZaMuEHotmrEMHZyZYK5tsOXOmG/O/pHPAVRsm6PD0Xbjm9YDofEVRWzGIrFRH+lQUcfIDvOuf50I" +
            "jjdxVJTatcO8J/tgShVKpVSKHY2Q82qeL4vRbwDwLry9c9PDV9dQAnt8o7NM16gJcWgPw+lO1smhTJs9lHgaW5+daurEgCp2jvNv5N2I8a+vEBoCHmlS/eim" +
            "g+1r0D9DTaDFzH8yWOWFKp48O4uoNU5RNK+ihjy0WMquoymBCj+UAbP/bPrTZemcuRtfAjHgFKdEZHNojeMlaV+YJcJ4NfJilDabf6OFzwEKk7epOHipKvqr" +
            "8A/twjBybOobzjmwft6LjrzINg7NaQ5M6coynKBVx/EyXcgyvPLtbRpZQ6y+TFnVwMLhxFWuDO5Y9rt+039X8aOFQuTkRo3HuDWhjvD8bEiRkTY14gdQ2Gap" +
            "jo3dCLZd72ezKpT2IQ3ydp6CddTJzZG97L0jIMqsHad5nBV5Sq/Ngvycl+tsmEOdRWOGYeig5uWbYCjhVM5NpDMLNasIRWtHVeA91UHnhKc3jGHz6+iY1P9m" +
            "oRLrJlFz/SNq/Dmx3E13XIgtV01ywm5156aN4o/Mk51PBmEV+o9mlRXYu0sn4tsTFupu1fPn1aQTPQXkuf79lh5Qu63pmxiAr/OYZFijK4lprfo8hp1s4hHS" +
            "yUIQvg6HvO7K0UmsIpEQyy9KeOP52ivH60Hpy0RtyT4+h/u1qwedVfUB2e7mmKPubHpsT+GHLendSiq6TRUBRx5YidhQmuvUDQCAXT1mtgoTh5KXWePOHWDY" +
            "WeJQ6e7rLLYD0mwwfiUq7k8WV2TRbXKx5c3NHkX96ys7GUVnWmv4NplmEcDJa0pIcR0KBo2M4Zx6kCaBYdvMYTlbglNyU/jmZAPmtCk3skvTplFkZNcx9J7w" +
            "ZzKCnhkFjhUrrCMc02lzAuy86p3jwH3/rcQgEwGTY9bsQt6y/LP/7Ek8Sg1b+7h9B7U78/N26XSyRmCZfVbFKOEzrSzbQDrEcagbJxqWYdvU9hVDNUFIj7hL" +
            "/rdMqlvN/sTj/LsxzZML+xKtZT2Qo7+GrxurBpxX4mCbW56p0vMVPLKj9Ch8dapyl5qoB84frmfRuEVN3/MVddPl8U0vlm8xETs7n4cKctV+7kogwFVhsu87" +
            "TLRDs3GnHo9q/glWvsBzDZU01QmHSfl2gF0zy1VNDoVjY++fM5x9Fd54cjQ2+IxvCMWsks//Wnjq6L7UYpaTPXbwnG+omG9varfa+BcaNDXj28qD3bMkqdVI" +
            "hfDpAB2GMyqfiNvZc2iywfW+W2jejw0fFiObCfqtUpSvhTsmj84/E/w63drxjt1AbXdIKwIi6gUG0AiQmacS9+cob4R+1DW2o5t8iYNZdrcPypeQuMafakkR" +
            "pC9bgaw7vsQLQ2B3HC007PF2gnt87IwSlkXz2b12WrYa5/tzFuQDrtzSqCzyNIgB+UvgbJH+ymKogsMWLMLMZPVfc27RqTdt+H626VAlu38UE/gqpK4WspfQ" +
            "222T/GEEJ2nAbRqKwkcqlA7s4WGuKcyteelENcf7kuwR0qezAGrQ6Kpj/ar85jhVX3KHDn1K7fZSE46vD/yVd+1GJsrR9uZIctZbP+apoaJm6B3e0DdSMUa6" +
            "31K2rro3B8jaC19TS65Dn8ZX0HOCmlNvhm18kvP/n7HMj3+nhIOUnmA+nL/mYB3b9WN+5cK5/5FdwXMd+k98O44TXH1xYmyHGQg65wdzr7UaFXt0FpDt+La0" +
            "exxIT72/SQw4XcCcY217dOv9dZ67UVKNeQ2xToBcUavqOyntadoac/N9NavVqwGQwm2lIL5XbDm1dn9DRhz70BwW5ZCRc7+X1zkS+TjGpV6jduvlIIey3JGF" +
            "4dz7tr/uyz78QqsXzh6wAtFfEfbVL0jpSQE/8MZq7/EkqTY86xcOzLr3G3K7Tr2Z8dRU+8Nr7eR6NubeoVlr7NWoZtdZpVtY7ed7MJ+jPS3aO16lhqRdStSH" +
            "mlOnoP+2aYHMKDhPvcL4Eo9jwmUP//i2VY4PUWF0SQlHg9FkUw30jj2wrE3LkbP/Pl3UhertQi7dRXWnRrEKnvRxnULfKrfxLVZm+r4lJBz0Eg7bl8MEi6ns" +
            "3/oYZT4QDQTsbqctnMFQ71vDzlDvanqtXqNg8UxpqvNgoD1+zvcM5ycjWe/T8sYxLX3flOtRw1GdAFsvqTNK1BTh7utUq+AyltEII3RAnuqIZRuFb6+4UVOx" +
            "nCcjnbC6A2OdfOaMQrJeX3dR1bJtdQvTVUp+kGEFZvqw3+tMPZuBncjve41OZczV5dylp31bmWHHebsyfIpLOX7DEUlEC99l4uAqiNGzlP9AvS0F3XbxwytT" +
            "K/P1f+oU2YVACna5kgPcHK3kq19qw+1EscOy18vw7nJqtVMf3/oNE1x55O7wHd/rBCip17HIS5syjGpNuhb3RJJOEeKY3ZKR2i6sJag9rAl1vQJy777R0TcA" +
            "DN874AtjvnMsf3rDIS/VVEjhpbWajKG9XzpYl/z7R2DyeEJAKpa7umtXjfRX2hCrfNstNXbsakm3kNj7Rz2wtNP5H8tsJGroo/JJg/4OiwLjfa8JBnmfGnns" +
            "FDN7YyiDAnWOma5dFww6c6CJWt5bE7GMHFBSnCaLRcyDzkV/vxIe1nD69b3BxzlppMwq48+/d/fcq+Lb9mjiln39twatHpme7Wt69GnODPPlDY5LI+3/msFC" +
            "Cp5OKd/VYt5EPcc3CD8152EO4Ilb899YU9+JiT8e9gY7xJItg8zqpZC5pqXKx5rN6cgnpm01o1KtOiPLRnTZEWWwp9fsGBtZSncIs7OX+H07wpUsx1a+4bCD" +
            "V13Awx4gWySo+koj1BTtekPgvN9bD/jpQX+AtJ5Q1ks7w6uCf/8Blk+o40l6AAA=";

    private static int[][] matrix() throws Exception {
        byte[] packed = Base64.getDecoder().decode(DATA);
        try (BufferedReader in = new BufferedReader(new InputStreamReader(
                new GZIPInputStream(new ByteArrayInputStream(packed)), StandardCharsets.US_ASCII))) {
            int[][] a = new int[80][80];
            for (int r = 0; r < 80; r++) {
                String[] fields = in.readLine().split(",");
                for (int c = 0; c < 80; c++) a[r][c] = Integer.parseInt(fields[c]);
            }
            return a;
        }
    }
    private static final class Node implements Comparable<Node> {
        final int r, c, cost;
        Node(int r, int c, int cost) { this.r = r; this.c = c; this.cost = cost; }
        public int compareTo(Node other) { return Integer.compare(cost, other.cost); }
    }
    public static void main(String[] args) throws Exception {
        int[][] a = matrix(), distance = new int[80][80];
        for (int[] row : distance) Arrays.fill(row, Integer.MAX_VALUE);
        PriorityQueue<Node> queue = new PriorityQueue<>();
        distance[0][0] = a[0][0]; queue.add(new Node(0, 0, a[0][0]));
        int[] dr = {-1, 1, 0, 0}, dc = {0, 0, -1, 1};
        while (!queue.isEmpty()) {
            Node cur = queue.remove();
            if (cur.cost != distance[cur.r][cur.c]) continue;
            if (cur.r == 79 && cur.c == 79) { System.out.println(cur.cost); return; }
            for (int k = 0; k < 4; k++) {
                int nr = cur.r + dr[k], nc = cur.c + dc[k];
                if (nr < 0 || nr == 80 || nc < 0 || nc == 80) continue;
                int next = cur.cost + a[nr][nc];
                if (next < distance[nr][nc]) { distance[nr][nc] = next; queue.add(new Node(nr, nc, next)); }
            }
        }
    }
}
