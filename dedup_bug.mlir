// RUN: circt-opt --firrtl-dedup %s | FileCheck %s

// CHECK-LABEL: firrtl.circuit "Top"
firrtl.circuit "Top" {
  firrtl.module @Top() {
    // CHECK: firrtl.instance foo @Foo()
    // CHECK: firrtl.instance bar @Bar()
    firrtl.instance foo @Foo()
    firrtl.instance bar @Bar()
  }

  // CHECK-LABEL: firrtl.module private @Foo()
  firrtl.module private @Foo() {
    %w = firrtl.wire : !firrtl.class<@Spam()>
  }
  
  // CHECK-LABEL: firrtl.module private @Bar()
  // Moduł @Bar nie powinien zostać usunięty, ponieważ zależy od innej klasy niż @Foo.
  firrtl.module private @Bar() {
    %w = firrtl.wire : !firrtl.class<@Eggs()>
  }

  // Obie klasy są publiczne, więc NIE powinny być deduplikowane.
  // CHECK: firrtl.class @Spam()
  // CHECK: firrtl.class @Eggs()
  firrtl.class @Spam() {}
  firrtl.class @Eggs() {}
}
