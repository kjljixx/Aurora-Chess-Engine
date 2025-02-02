Try to implement Alg. 2 from https://arxiv.org/pdf/0805.3415

Commit 79345348dd94660531c60b416ed269a32002857c (no bench change, add infrastructure needed) is non-regressor (https://kjljixx.pythonanywhere.com/test/272/)

Really bad with (relatively) small discount factors, like 0.99998 (https://kjljixx.pythonanywhere.com/test/273/)

Much better with higher discount factors, like 0.99999 (https://kjljixx.pythonanywhere.com/test/275/)