
#ifndef CALLBACK_H_
#define CALLBACK_H_


// TODO some doc
struct callback {
    virtual ~callback() {}
    virtual void run() = 0;
};


// TODO some doc
template<class T>
struct callback1 {
    virtual ~callback1() {}
    virtual void run(T t) = 0;
};


template<class O, class R>
class member_ptr_callback0 : public callback {
public:
    explicit member_ptr_callback0(O *obj, R (O::*ptr)())
        : obj_(obj), ptr_(ptr)
    {
    }

    void run()
    {
        (obj_->*ptr_)();
        delete this;
    }

private:
    member_ptr_callback0(const member_ptr_callback0 &);
    member_ptr_callback0 &operator= (const member_ptr_callback0 &);

private:
    O *obj_;
    R (O::*ptr_)();
};


// TODO generate all these helper classes and functions.


template<class O, class P, class R>
class member_ptr_callback1 : public callback {
public:
    explicit member_ptr_callback1(O *obj, R (O::*ptr)(P), P param)
        : obj_(obj), ptr_(ptr), param_(param)
    {
    }

    void run()
    {
        (obj_->*ptr_)(param_);
        delete this;
    }

private:
    member_ptr_callback1(const member_ptr_callback1 &);
    member_ptr_callback1 &operator= (const member_ptr_callback1 &);

private:
    O *obj_;
    R (O::*ptr_)(P);
    P param_;
};


template<class O, class P1, class P2, class R>
class member_ptr_callback2 : public callback {
public:
    explicit member_ptr_callback2(
            O *obj,
            R (O::*ptr)(P1, P2), P1 param1, P2 param2)
        : obj_(obj), ptr_(ptr), param1_(param1), param2_(param2)
    {
    }

    void run()
    {
        (obj_->*ptr_)(param1_, param2_);
        delete this;
    }

private:
    member_ptr_callback2(const member_ptr_callback2 &);
    member_ptr_callback2 &operator= (const member_ptr_callback2 &);

private:
    O *obj_;
    R (O::*ptr_)(P1, P2);
    P1 param1_;
    P2 param2_;
};


template<class T, class O, class R>
class member_ptr_callback0_1 : public callback1<T> {
public:
    explicit member_ptr_callback0_1(O *obj, R (O::*ptr)(T))
        : obj_(obj), ptr_(ptr)
    {
    }

    void run(T t)
    {
        (obj_->*ptr_)(t);
        delete this;
    }

private:
    member_ptr_callback0_1(const member_ptr_callback0_1 &);
    member_ptr_callback0_1 &operator= (const member_ptr_callback0_1 &);

private:
    O *obj_;
    R (O::*ptr_)(T);
};


template<class O, class R>
callback *make_callback(O *obj, R (O::*ptr)())
{
    return new member_ptr_callback0<O, R>(obj, ptr);
}


template<class O, class P, class R>
callback *make_callback(O *obj, R (O::*ptr)(P), P param)
{
    return new member_ptr_callback1<O, P, R>(obj, ptr, param);
}


template<class O, class P1, class P2, class R>
callback *make_callback(O *obj, R (O::*ptr)(P1, P2), P1 param1, P2 param2)
{
    return new member_ptr_callback2<O, P1, P2, R>(obj, ptr, param1, param2);
}


template<class T, class O, class R>
callback1<T> *make_callback(O *obj, R (O::*ptr)(T))
{
    return new member_ptr_callback0_1<T, O, R>(obj, ptr);
}

#endif

