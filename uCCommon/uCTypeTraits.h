#ifndef ucTypeTraits_H_INCLUDED
#define ucTypeTraits_H_INCLUDED

namespace ucstd{
	
	template<bool B, class T = void>
	struct enable_if {};
 
	template<class T>
	struct enable_if<true, T> { typedef T type; };
	
	template<bool B, class T, class F>
	struct conditional { typedef T type; };

	template<class T, class F>
	struct conditional<false, T, F> { typedef F type; };
	
	template<typename T>
	T clamp(T value, T min, T max){
		return value<min?min:(value>max?max:value);
	}
	
	template<typename T>
	T max(T a, T b){
		return a>b?a:b;
	}
	
	struct true_type { typedef bool value_type; static constexpr bool value = true; };
	
	struct false_type { typedef bool value_type; static constexpr bool value = false; };
	
	template<class T, class U>
	struct is_same : false_type {};
	 
	template<class T>
	struct is_same<T, T> : true_type {};
	
	template< class T > struct remove_pointer                    {typedef T type;};
	template< class T > struct remove_pointer<T*>                {typedef T type;};
	template< class T > struct remove_pointer<T* const>          {typedef T type;};
	template< class T > struct remove_pointer<T* volatile>       {typedef T type;};
	template< class T > struct remove_pointer<T* const volatile> {typedef T type;};
	
	template< class T > struct remove_const                { typedef T type; };
	template< class T > struct remove_const<const T>       { typedef T type; };
	
	template< class T > struct remove_reference      { typedef T type; };
	template< class T > struct remove_reference<T&>  { typedef T type; };
	template< class T > struct remove_reference<T&&> { typedef T type; };
	
};

#endif
