#ifndef __DAVAENGINE_INTROSPECTION_BASE_H__
#define __DAVAENGINE_INTROSPECTION_BASE_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
	class IntrospectionInfo;
	class IntrospectionCollectionBase;
	class KeyedArchive;
	struct MetaInfo;

	// Базовое представление члена интроспекции
	class IntrospectionMember
	{
		friend class IntrospectionInfo;

	public:
		IntrospectionMember(const char *_name, const char *_desc, const int _offset, const MetaInfo *_type, int _flags = 0)
			: name(_name), desc(_desc), offset(_offset), type(_type), flags(_flags)	
		{ }

		// Имя члена интроспекции, соответствует имени члена класса
		const char* Name() const
		{
			return name;
		}

		// Описание члена интроспекции, произвольно указанное пользователем при объявлении интроспекции
		const char* Desc() const
		{
			return desc;
		}

		// Возвдащает мета-тип члена интроспекции
		const MetaInfo* Type() const
		{
			return type;
		}

		// Возвращает указатель непосдедственно на данные члена интроспекции
		// Следует учитывать, что если членом интроспекции является указатель, 
		// то данная функция вернет указатель на указатель
		// Проверить является ли член интроспекции указателем можно по его мета-информации:
		//   MetaInfo* meta = member->Type();
		//   meta->IsPointer();
		void* Pointer(void *object) const
		{
			return (((char *) object) + offset);
		}

		// Возвращает вариант данных члена интроспекции. Имлементация варианта должна поддерживать
		// создание из данных, определяемыт мета-типом данного члена интроспекции.
		virtual VariantType Value(void *object) const
		{
			return VariantType::LoadData(Pointer(object), type);
		}

		// Устанавливает данные члена интроспекции из указанного варианта. 
		virtual void SetValue(void *object, const VariantType &val) const
		{
			VariantType::SaveData(Pointer(object), type, val);
		}

		// Возвращает данные члена интроспекции в виде коллекции
		virtual const IntrospectionCollectionBase* Collection() const
		{
			return NULL;
		}

		const int Flags() const
		{
			return flags;
		}

	protected:
		const char* name;
		const char *desc;
		const int offset;
		const MetaInfo* type;
		const int flags;
	};

	// Базовое представление члена интроспекции, являющегося коллекцией
	class IntrospectionCollectionBase : public IntrospectionMember
	{
	public:
		typedef void* Iterator;

		IntrospectionCollectionBase(const char *_name, const char *_desc, const int _offset, const MetaInfo *_type, int _flags = 0)
			: IntrospectionMember(_name, _desc, _offset, _type, _flags)
		{ }

		virtual MetaInfo* CollectionType() const = 0;
		virtual MetaInfo* ValueType() const = 0;
		virtual int Size(void *object) const = 0;
		virtual Iterator Begin(void *object) const = 0;
		virtual Iterator Next(Iterator i) const = 0;
		virtual void Finish(Iterator i) const = 0;
		virtual void ItemValueGet(Iterator i, void *itemDst) const = 0;
		virtual void ItemValueSet(Iterator i, void *itemSrc) = 0;
		virtual void* ItemPointer(Iterator i) const = 0;
	};

	// Вспомогательный класс для определения содержит ли указанный шаблонный тип интроспекцию
	// Наличие интроспекции определяется по наличию функции GetTypeInfo у данного типа
	template<typename T> 
	class HasIntrospection
	{
		class yes {	char m; };
		class no { yes m[2]; };

		// Базовый класс для проверки, содержит пустую искомую функцию
		struct TestBase
		{ 
			const IntrospectionInfo* GetTypeInfo();
		};

		// Для проверки типа Т мы создаем склаа наследованный от TestBase и Т
		// Таким образом если Т содержит функцию GetTypeInfo, то Test::GetTypeInfo будет указывать
		// на Т::GetTypeInfo, иначе на TestBase::GetTypeInfo
		struct Test : public T, public TestBase 
		{};

		// Вспомогательный класс для вывода типов
		template <typename C, C c> struct Helper
		{};

		// Функция для проверки является содержит заданный тип U имплементацию функции TestBase::GetTypeInfo
		// Компилятор сможет вывести типы для класса Helper только в том случае если &U::GetTypeInfo соответствует 
		// указателю на функцию GetTypeInfo, являющуюся членом класса TestBase
		template <typename U> 
		static no Check(U*, Helper<const IntrospectionInfo* (TestBase::*)(), &U::GetTypeInfo>* = 0); 

		// В случае когда вывод типов невозможен для первой функции, будет вызвана эта. Это произойдет только тогда, 
		// когда Т содержит свою функцию GetTypeInfo, а следовательно содержит интроспекцию
		static yes Check(...);

	public: 
		// Статическая переменна, значение которой будет равно true в случае,
		// когда тип Т содержит интроспекцию
		static const bool result = (sizeof(yes) == sizeof(Check((Test*)(0))));

		// Статическая функция для автоматического вывода типа Т по
		// переданной ссылке. 
		static const bool resultByObject(const T &t)
		{
			return HasIntrospection<T>::result;
		}
	};


	// Параметризированные имплементации HasIntrospection для базовых типов 
	// (так как наследование класса Test от базового типа невозможно)
	template<> class HasIntrospection<void> { public: static const bool result = false; };
	template<> class HasIntrospection<bool> { public: static const bool result = false; };
	template<> class HasIntrospection<char8> { public: static const bool result = false; };
	template<> class HasIntrospection<char16> { public: static const bool result = false; };
	template<> class HasIntrospection<int8> { public: static const bool result = false; };
	template<> class HasIntrospection<uint8> { public: static const bool result = false; };
	template<> class HasIntrospection<int16> { public: static const bool result = false; };
	template<> class HasIntrospection<uint16> { public: static const bool result = false; };
	template<> class HasIntrospection<int32> { public: static const bool result = false; };
	template<> class HasIntrospection<uint32> { public: static const bool result = false; };
	template<> class HasIntrospection<int64> { public: static const bool result = false; };
	template<> class HasIntrospection<uint64> { public: static const bool result = false; };
	template<> class HasIntrospection<float32> { public: static const bool result = false; };
	template<> class HasIntrospection<float64> { public: static const bool result = false; };
	template<> class HasIntrospection<KeyedArchive *> { public: static const bool result = false; };

	// Глобальная шаблонная функция(#1) для получения интроспекции заданного типа
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет true
	template<typename T> 
	typename EnableIf<HasIntrospection<T>::result, const IntrospectionInfo*>::type GetIntrospection() 
	{
		return T::TypeInfo();
	}

	// Глобальная шаблонная функция(#2) для получения интроспекции заданного типа
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
	template<typename T>
	typename EnableIf<!HasIntrospection<T>::result, const IntrospectionInfo*>::type GetIntrospection() 
	{
		return NULL;
	}

	// Глобальная шаблонная функция(#3) для получения интроспекции заданного объекта. 
	// Тип объекта будет выведен компилятором автоматически.
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет true
	// Пример:
	//
	// class A {};
	// class B : public class A { ... INTROSPECTION ... }
	// B *b;
	// GetIntrospection(b)			// вернет интроспекцию класса B
	// GetIntrospection((A *) b)	// вернет NULL, т.к. будет вызвана функция #4, см. ниже
	//
	template<typename T> 
	typename EnableIf<HasIntrospection<T>::result, const IntrospectionInfo*>::type GetIntrospection(const T *t) 
	{
		const IntrospectionInfo* ret = NULL;

		if(NULL != t)
		{
			ret = t->GetTypeInfo();
		}

		return ret;
	}

	// Глобальная шаблонная функция(#4) для получения интроспекции заданного объекта. 
	// Тип объекта будет выведен компилятором автоматически.
	// Функция скомпилируется только для тех типов, для которых HasIntrospection<T>::result будет false
	template<typename T>
	typename EnableIf<!HasIntrospection<T>::result, const IntrospectionInfo*>::type GetIntrospection(const T *t) 
	{
		return NULL;
	}
};

#endif // __DAVAENGINE_INTROSPECTION_BASE_H__